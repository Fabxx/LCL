#include "core.hpp"
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "libretro.h"

#include <iostream>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

static uint32_t* frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

// libcurl callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

//TODO: make a system to let the user choose the base path of retro arch.
core::core()
{
    std::string retreived_path;

#ifdef _WIN32
    _directories = {
        "C:\\RetroArch-Win64\\system\\azahar",
        "C:\\RetroArch-Win64\\thumbnails\\Nintendo - Nintendo 3DS",
        "C:\\RetroArch-Win64\\thumbnails\\Nintendo - Nintendo 3DS\\Named_Boxarts",
        "C:\\RetroArch-Win64\\thumbnails\\Nintendo - Nintendo 3DS\\Named_Snaps",
        "C:\\RetroArch-Win64\\thumbnails\\Nintendo - Nintendo 3DS\\Named_Titles"
    };

    _downloaderDirs = {
        "C:\\RetroArch-Win64\\system\\azahar\\0.Url.txt",
        "C:\\RetroArch-Win64\\system\\azahar\\1.CurrentVersion.txt",
        "C:\\RetroArch-Win64\\system\\azahar\\2.NewVersion.txt",
    };

    _executable = "C:\\RetroArch-Win64\\system\\azahar\\azahar.exe";
    _asset_id = 4;

#endif

    _urls = {
         "https://api.github.com/repos/azahar-emu/azahar/releases/latest",
         "https://github.com/azahar-emu/azahar/releases"
    };
}

bool core::retro_core_setup()
{
    for (auto& path : _directories) {
        if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
			log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Created directory: %s\n", path.c_str());
        } else {
			log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Directory already exists: %s\n", path.c_str());
        }
    }

    if (std::filesystem::exists(_executable)) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Found Emulator in: %s\n", _executable.c_str());
        return true;
    }

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Downloading emulator.\n");
    return false;
}

bool core::retro_core_downloader()
{
    CURL* curl = curl_easy_init();
    std::string jsonResponse;

    if (!curl) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to initialize cURL.\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, _urls[0].c_str());

    struct curl_slist* headers = nullptr;
    
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: curl/7.88.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] cURL error: %s\n", curl_easy_strerror(res));
        return false;
    }

    if (jsonResponse.empty()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Empty response from server.\n");
        return false;
    } else {
		log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Received JSON response: %s\n", jsonResponse.c_str());
    }

    json parsed;

    try {
        parsed = json::parse(jsonResponse);
    } catch (json::exception e) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] JSON parsing error: %s\n", e.what());
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Response: %s\n", jsonResponse.c_str());
        return false;
    }

    if (!parsed.contains("tag_name") || !parsed.contains("assets")) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] JSON does not contain expected fields.\n");
        return false;
    }

    std::string tag = parsed["tag_name"];
    const auto& assets = parsed["assets"];

    if (assets.size() <= _asset_id) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Asset ID %d is out of range.\n", _asset_id);
        return false;
    }

    std::string name = assets[_asset_id]["name"];
    int id = assets[_asset_id]["id"];
    std::string url = _urls[1] + tag + "/" + name;

    std::ofstream urlOut(_downloaderDirs[0]);
    std::ofstream idOut(_downloaderDirs[1]);

    if (!urlOut || !idOut) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to open output files.\n");
        return false;
    }

    urlOut << url << "\n";
    idOut << id << "\n";

    urlOut.close();
    idOut.close();

    // Scarica l'emulatore
    std::string outputFile;

#ifdef _WIN32
    outputFile = _directories[0] + "\\azahar.zip";
#endif

    // Scarica il file binario
    curl = curl_easy_init();
    if (!curl) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to initialize cURL for download.\n");
        return false;
    }

    FILE* outFile = fopen(outputFile.c_str(), "wb");
    if (!outFile) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to open output file for writing: %s\n", outputFile.c_str());
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(outFile);

    if (res != CURLE_OK) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] cURL download error: %s\n", curl_easy_strerror(res));
       return false;
    }
	log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Downloaded emulator to: %s\n", outputFile.c_str());
    return true;
}

bool core::retro_core_updater()
{
    // This function is not implemented in this example.
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core updater is not implemented.\n");
    return true;
}

bool core::retro_core_extractor()
{
    // This function is not implemented in this example.
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core extractor is not implemented.\n");
    return true;
}

static void fallback_log(enum retro_log_level level, const char* fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

void retro_init(void)
{
    frame_buf = (uint32_t*)calloc(320 * 240, sizeof(uint32_t));
}

void retro_deinit(void)
{
    free(frame_buf);
    frame_buf = NULL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info* info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "Azahar Launcher";
    info->library_version = "0.1a";
    info->need_fullpath = true;
    info->valid_extensions = "3ds|3dsx|elf|axf|cci|cxi|app";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static struct retro_system_timing system_timing = {
   .fps = 60.0,
   .sample_rate = 44100.0
};

static struct retro_game_geometry geometry = {
   .base_width = 320,
   .base_height = 240,
   .max_width = 320,
   .max_height = 240,
   .aspect_ratio = 4.0f / 3.0f
};

void retro_get_system_av_info(struct retro_system_av_info* info)
{
    info->timing = system_timing;
    info->geometry = geometry;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    bool no_content = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_reset(void)
{
    // Nothing needs to happen when the game is reset.
}

void retro_run(void)
{
    // Clear the display.
    unsigned stride = 320;
    video_cb(frame_buf, 320, 240, stride << 2);

    // Shutdown the environment now that xemu has loaded and quit.
    environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

bool retro_load_game(const struct retro_game_info* info)
{
    core core;

    if (!core.retro_core_setup()) {
        core.retro_core_downloader();
    }
    return true;
}

void retro_unload_game(void)
{
    // Nothing needs to happen when the game unloads.
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info)
{
    return retro_load_game(info);
}

size_t retro_serialize_size(void)
{
    return 0;
}

bool retro_serialize(void* data_, size_t size)
{
    return true;
}

bool retro_unserialize(const void* data_, size_t size)
{
    return true;
}

void* retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0;
}

void retro_cheat_reset(void)
{
    // Nothing.
}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
    (void)index;
    (void)enabled;
    (void)code;
}