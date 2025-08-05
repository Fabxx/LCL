#include "core.hpp"
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

core::core()
{
   _base_path = std::filesystem::current_path();
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Using current path: %s\n", _base_path.string().c_str());

    _directories = {
         (_base_path / "system" / "azahar").string(),
         (_base_path / "thumbnails" / "Nintendo - Nintendo 3DS").string(),
         (_base_path / "thumbnails" / "Nintendo - Nintendo 3DS" / "Named_Boxarts").string(),
         (_base_path / "thumbnails" / "Nintendo - Nintendo 3DS" / "Named_Snaps").string(),
         (_base_path / "thumbnails" / "Nintendo - Nintendo 3DS" / "Named_Titles").string()
    };

    _downloaderDirs = {
        (_base_path / "system" / "azahar" / "0.Url.txt").string(),
        (_base_path / "system" / "azahar" / "1.CurrentVersion.txt").string(),
        (_base_path / "system" / "azahar" / "2.NewVersion.txt").string(),
        (_base_path / "system" / "azahar" / "azahar.zip").string()
    };

#ifdef _WIN32
    _asset_id = 4;
    _executable = (_base_path / "system" / "azahar" / "azahar.exe").string();
#elif __APPLE__
    _asset_id = 4;
    _executable = (_base_path / "system" / "azahar" / "azahar").string();
#else
    _asset_id = 4;
    _executable = (_base_path / "system" / "azahar" / "azahar").string();
#endif
    
	_url_asset_id = 0;

    _urls = {
         "https://api.github.com/repos/azahar-emu/azahar/releases/latest",
         "https://github.com/azahar-emu/azahar/releases/download"
    };
}

bool core::check_retroarch_path()
{
    if (_base_path.empty()) {
        return false;
	}

    return true;
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

bool core::set_url(CURL* curl, CURLcode& res)
{
    std::string jsonResponse;

    if (!curl) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to initialize cURL.\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, _urls[_url_ids::LATEST_RELEASE_URL].c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: curl/7.88.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (res != CURLE_OK || jsonResponse.empty()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to fetch metadata: %s\n", curl_easy_strerror(res));
        return false;
    }

    json parsed;

    try {
        parsed = json::parse(jsonResponse);
    }
    catch (const json::exception& e) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] JSON parse error: %s\n", e.what());
        return false;
    }

    if (!parsed.contains("tag_name") || !parsed.contains("assets") || !parsed["assets"].is_array()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Invalid JSON structure.\n");
        return false;
    }

    _tag = parsed["tag_name"];
    const auto& assets = parsed["assets"];

    if (_asset_id >= assets.size()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Asset ID %d out of range.\n", _asset_id);
        return false;
    }

    std::string name = assets[_asset_id]["name"];
    _url_asset_id = assets[_asset_id]["id"];
    _urls.push_back(_urls[_url_ids::BASE_URL] + "/" + _tag + "/" + name);

    // export final download URL

    std::ofstream urlOut(_downloaderDirs[_downloader_ids::URL_FILE]);

    if (urlOut.is_open()) {
        urlOut << _urls[_url_ids::DOWNLOAD_URL] << "\n";
        urlOut.close();
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Download URL saved: %s\n", _urls[_url_ids::DOWNLOAD_URL].c_str());
    }
    else {
        log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not write to URL file: %s\n", _downloaderDirs[_downloader_ids::URL_FILE].c_str());
    }
}

bool core::download(CURL *curl, CURLcode& res, std::string &url)
{
    if (!curl) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to initialize cURL for download.\n");
        return false;
    }

    FILE* outFile = fopen(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE].c_str(), "wb");

    if (!outFile) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to open file: %s\n",
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE].c_str());
        return false;
    }

    struct curl_slist* download_headers = nullptr;
    download_headers = curl_slist_append(download_headers, "User-Agent: curl/7.88.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, download_headers);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    res = curl_easy_perform(curl);
    curl_slist_free_all(download_headers);
    curl_easy_cleanup(curl);
    fclose(outFile);

    if (res != CURLE_OK) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to download file: %s\n", curl_easy_strerror(res));
        return false;
    }

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Download complete: %s\n", 
        _downloaderDirs[_downloader_ids::DOWNLOADED_FILE].c_str());
    return true;
}

bool core::retro_core_downloader()
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    std::string newVersionStr;
    std::string currentVersionStr;

    if (!set_url(curl, res)) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to set URL.\n");
        return false;
    }

    bool firstBoot = !std::filesystem::exists(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

    if (firstBoot) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] First boot detected, downloading emulator...\n");

        if (!download(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
			log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to download emulator.\n");
            return false;
        }

        // Export current version ID in both files.
        std::ofstream currentOut(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
        std::ofstream newOut(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

        if (currentOut.is_open() && newOut.is_open()) {
            currentOut << newVersionStr << "\n";
            newOut << newVersionStr << "\n";
        }
        else {
            log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not write version files on first boot.\n");
        }

        return true;
    }

    // If it's not the first boot, check for updates.
    std::ifstream currentIn(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
    std::ifstream newIn(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

    if (!currentIn.is_open() || !newIn.is_open()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Could not read version files.\n");
        return false;
    }

    std::getline(currentIn, currentVersionStr);
    std::getline(newIn, newVersionStr);

    if (currentVersionStr != newVersionStr) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] New version detected (current: %s, new: %s). Downloading update...\n",
            currentVersionStr.c_str(), newVersionStr.c_str());

        if (!download(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
			log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to download update.\n");
            return false;
        }

        // Aggiorna i file versione
        std::ofstream currentOut(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
        std::ofstream newOut(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

        if (currentOut.is_open() && newOut.is_open()) {
            currentOut << newVersionStr << "\n";
            newOut << newVersionStr << "\n";
        }
        else {
            log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not update version files after download.\n");
        }
    }
    else {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core is already up to date (version: %s).\n", currentVersionStr.c_str());
		std::filesystem::remove(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE]);
    }

    return true;
}

bool core::retro_core_extractor()
{
    return true;
}

bool core::retro_core_boot(struct retro_system_info* info)
{
    // This function is not implemented in this example.
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core boot is not implemented.\n");
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

    if (!core.check_retroarch_path()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Invalid Retroarch path.\n");
        return false;
	}

	// if first boot download emulator, else check for updates
    if (!core.retro_core_setup()) {
        core.retro_core_downloader();
        core.retro_core_extractor();
    }
    else {
        core.retro_core_downloader();
        core.retro_core_extractor();
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