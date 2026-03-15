#include "lcl_utils.hpp"
#include "libretro.h"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <format>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <system_error>
#include <unordered_set>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static uint32_t* frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

#ifdef CORE
#ifdef SYSTEM_NAME
std::string core_name = CORE;
std::string system_name = SYSTEM_NAME;
#endif
#endif

static std::string g_emu_extensions;

// libcurl callback
static inline size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

lcl_utils::lcl_utils() {
    _is_flatpak = false;
    _base_path = std::filesystem::current_path();
    _config_path = (_base_path / "LCL.cfg").string();
    _url_asset_id = 0;

    _directories = {
         (_base_path / "system" / core_name).string(),
         (_base_path / "thumbnails" / system_name).string(),
         (_base_path / "thumbnails" / system_name / "Named_Boxarts").string(),
         (_base_path / "thumbnails" / system_name / "Named_Snaps").string(),
         (_base_path / "thumbnails" / system_name / "Named_Titles").string()
    };

    _downloaderDirs = {
        (_base_path / "system" / core_name / "0.Url.txt").string(),
        (_base_path / "system" / core_name / "1.CurrentVersion.txt").string(),
        (_base_path / "system" / core_name / "2.NewVersion.txt").string()
    };

#ifdef __linux__
    lcl_check_flatpak();
#endif
}

#ifdef __linux__
bool lcl_utils::lcl_check_flatpak() {

    if (std::getenv("container") != nullptr) {
        const char *flatpak_path = "/run/host/usr/bin/fusermount";
        const char *flatpak_var = "FUSERMOUNT_PROG";
        int flag = 1;

        _is_flatpak = true;

        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] User under flatpak, setting env variable %s to %s\n", flatpak_var, flatpak_path);

        if (setenv(flatpak_var, flatpak_path, flag) == EXIT_SUCCESS) {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Setup done.\n");
        } else {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Flatpak setup failed with error %d\n.", errno);
        }
    }
}
#endif

bool lcl_utils::lcl_check_config_file() {
    
    bool is_config_available = false;
    auto ini_path = (_base_path / "LCL.cfg").string();

    if (!std::filesystem::exists(ini_path)) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Configuration file not found, aborting.\n");
        return is_config_available;
    }

    _cfg.load(ini_path);

    if (!_cfg.contains(core_name)) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Section %s not found.\n", _cfg_section);
        return is_config_available;
    }

    _cfg_section = _cfg[core_name];

    is_config_available = true;

    return is_config_available;
}

bool lcl_utils::lcl_setup_config_params() {

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Loading configuration from %s\n", _config_path.c_str());

#ifdef _WIN32
    _search_token = _cfg_section["WINDOWS_SEARCH_TOKEN"].as<std::string>();
    _downloaderDirs.push_back((_base_path / "system" / core_name / _cfg_section["ARCHIVE"].as<std::string>()).string());
    _executable = (_base_path / "system" / core_name / _cfg_section["WIN_EXECUTABLE"].as<std::string>()).string();
#elif __linux__
    _search_token = _cfg_section["LINUX_SEARCH_TOKEN"].as<std::string>();
    _executable = (_base_path / "system" / core_name / _cfg_section["LINUX_EXECUTABLE"].as<std::string>()).string();
    _downloaderDirs.push_back((_executable));
#endif

    _urls.push_back(_cfg_section["API_URL"].as<std::string>());
    _urls.push_back(_cfg_section["GIT_URL"].as<std::string>());

    if (_urls.empty()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Could not fetch urls.\n");
        return false;
    }

    _emu_extensions = _cfg_section["EXTENSIONS"].as<std::string>();
    g_emu_extensions = _emu_extensions; // export extensions for retro_system_info struct.

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Loaded url config from LCL.cfg\n");
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] API URL: %s\n", _urls[LATEST_RELEASE_URL].c_str());
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] GIT URL: %s\n", _urls[BASE_URL].c_str());
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Loaded archive search token from LCL.cfg\n");
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Search Token: %s\n", _search_token.c_str());

    return true;
}

bool lcl_utils::lcl_setup_dirs()
{
    for (const auto& path : _directories) {
        if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
			log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Created directory: %s\n", path.c_str());
        } else {
			log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Directory already exists: %s\n", path.c_str());
        }
    }

    // If executing windows shortcuts there is no need to search or download an emulator.
    if (core_name != "windows") {
        if (std::filesystem::exists(_executable)) {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Found Emulator in: %s\n", _executable.c_str());
            return false;
        }

        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Downloading emulator.\n");
    }

    return true;
}

bool lcl_utils::lcl_build_download_url(CURL* curl, CURLcode& res)
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
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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

    std::string download_url;

    for (const auto& asset : assets) {
        std::string name = asset["name"];

        if (name.find(_search_token) != std::string::npos) {
            download_url = asset["browser_download_url"];
            _url_asset_id = asset["id"];
            break;
        }
    }

    if (download_url.empty()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] No asset found matching token: %s\n", _search_token.c_str());
        return false;
    }

    _current_version = std::to_string(_url_asset_id);
    _new_version = std::to_string(_url_asset_id);

    // write final download url into vector
    _urls.push_back(download_url);

    // export final download URL
    std::ofstream urlOut(_downloaderDirs[_downloader_ids::URL_FILE]);

    if (urlOut.is_open()) {
        urlOut << _urls[_url_ids::DOWNLOAD_URL] << "\n";
        urlOut.close();
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Download URL saved: %s\n", _urls[_url_ids::DOWNLOAD_URL].c_str());
    }
    else {
        log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not write to URL file: %s\n", _downloaderDirs[_downloader_ids::URL_FILE].c_str());
        return false;
    }

    return true;
}

bool lcl_utils::lcl_download_asset(CURL *curl, CURLcode& res, std::string &url)
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

bool lcl_utils::lcl_core_get()
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    if (!lcl_build_download_url(curl, res)) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to set URL.\n");
        return false;
    }

    if (!std::filesystem::exists(_executable)) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] First boot detected, downloading emulator...\n");

        if (!lcl_download_asset(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
			log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to download emulator.\n");
            return false;
        }

        // Export current version ID in both files.
        std::ofstream currentOut(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
        std::ofstream newOut(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

        if (currentOut.is_open() && newOut.is_open()) {
            currentOut << _current_version << "\n";
            newOut << _current_version << "\n";
            currentOut.close();
            newOut.close();
        }
        else {
            log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not write version files on first boot.\n");
        }

        return true;
    }

    // If it's not the first boot, check for updates.

    std::ifstream currentIn(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
    std::ofstream newOut(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

    if (!currentIn.is_open() || !newOut.is_open()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Could not read version files.\n");
        return false;
    }

    std::getline(currentIn, _current_version);

    if (newOut.is_open()) {
        newOut << _new_version << "\n";
		newOut.close();
    }
    else {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Could not write new version file.\n");
        return false;
    }

    if (_current_version != _new_version) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] New version detected (current: %s, new: %s). Downloading update...\n",
            _current_version.c_str(), _new_version.c_str());

        if (!lcl_download_asset(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
			log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to download update.\n");
            return false;
        }

        // Update versions
        std::ofstream currentOut(_downloaderDirs[_downloader_ids::CURRENT_VERSION_FILE]);
        std::ofstream newOut(_downloaderDirs[_downloader_ids::NEW_VERSION_FILE]);

        if (currentOut.is_open() && newOut.is_open()) {
            currentOut << _new_version << "\n";
            newOut << _new_version << "\n";
			currentOut.close();
            newOut.close();
        }
        else {
            log_cb(RETRO_LOG_WARN, "[LAUNCHER-WARN] Could not update version files after download.\n");
            return false;
        }
    }
    else {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core is already up to date (version: %s).\n", _current_version.c_str());
        return false;
    }

    return true;
}

#ifdef _WIN32
bool lcl_utils::lcl_core_extractor()
{
    const std::filesystem::path archive_path = _downloaderDirs[_downloader_ids::DOWNLOADED_FILE];
    const std::string ext = archive_path.extension().string();
    std::string command{};

    const std::unordered_set<std::string_view> supported_exts = { ".zip", ".7z" };
    const std::unordered_set<std::string_view> zip_emulators = { "azahar", "duckstation", "melonds", "xemu", "xenia" };
    const std::unordered_set<std::string_view> seven_zip_emulators = { "mgba", "pcsx2" };

    if (!supported_exts.contains(ext)) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Not an archive, aborting extraction.\n");
        return false;
    }

	// On windows use PowerShell
    if (zip_emulators.contains(core_name)) {
		log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Extracting emulator from ZIP archive.\n");
        command = std::format(
            "powershell -Command \""
            "Expand-Archive -Path '{}' -DestinationPath '{}' -Force; "
            "Remove-Item -Path '{}' -Force; "
            "$subfolder = Get-ChildItem -Path '{}' -Directory | Select-Object -First 1; "
            "if ($subfolder) {{ "
            "Copy-Item -Path \\\"$($subfolder.FullName)\\\\*\\\" -Destination '{}' -Recurse -Force; "
            "Remove-Item -Path $subfolder.FullName -Recurse -Force; "
            "}}"
            "\"",
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH]
        );

        system(command.c_str());
        std::filesystem::remove(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE]);
    }
    // Use 7z4PowerShell if 7z
    else if (seven_zip_emulators.contains(core_name)) {
		log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: 7z archive detected.\n");
        
        command = std::format(
            "powershell -Command \""
            "if ($null -ne (Get-Module -ListAvailable -Name 7Zip4PowerShell)) {{ exit 0 }} else {{ exit 1 }}\""
        );

        if (system(command.c_str()) == 0) {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Found 7z4Powershell module, skipping installation.\n");
        }
        else {
            command = std::format(
                "powershell -Command \""
                "Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser; "
                "Set-PSRepository -Name 'PSGallery' -InstallationPolicy Trusted; "
                "Install-Module -Name 7Zip4PowerShell -Force -Scope CurrentUser\""
            );

            if (system(command.c_str()) != 0) {
                log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR]: Failed to install 7z module, aborting.\n");
                return false;
            }
            else {
                log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: 7z module installed, extracting emulator.\n");
            }
        }

        command = std::format(
            "powershell -Command \""
            "Expand-7zip -ArchiveFileName '{}' -TargetPath '{}'; "
            "Remove-Item -Path '{}' -Force; "
            "$subfolder = Get-ChildItem -Path '{}' -Directory | Select-Object -First 1; "
            "if ($subfolder) {{ "
            "Copy-Item -Path \\\"$($subfolder.FullName)\\\\*\\\" -Destination '{}' -Recurse -Force; "
            "Remove-Item -Path $subfolder.FullName -Recurse -Force; "
            "}}"
            "\"",
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH]
        );

        system(command.c_str());
        std::filesystem::remove(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE]);
    }

    return true;
} 

#elif __linux__
bool lcl_utils::lcl_core_extractor()
{
    const std::filesystem::path archive_path = _downloaderDirs[_downloader_ids::DOWNLOADED_FILE];
    const std::string ext = archive_path.extension().string();
    std::string command{};

    const std::unordered_set<std::string_view> supported_exts = { ".zip", ".7z", ".tar"};
    const std::unordered_set<std::string_view> zip_emulators = { "azahar", "duckstation", "melonds", "xemu", "xenia" };
    const std::unordered_set<std::string_view> seven_zip_emulators = { "mgba", "pcsx2" };

    if (ext == ".AppImage") {
        command = std::format("chmod +x '{}'", _executable);
        if (system(command.c_str())) {
            log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Permission set for appImage.\n");
            return true;
        } else {
            log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to set permission for appImage.\n");
            return false;
        }
    }

    if (!supported_exts.contains(ext)) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Unsupported file type, aborting extraction.\n");
        return false;
    }

    if (zip_emulators.contains(core_name)) {
        command = std::format(
            "unzip -o '{}' -d '{}'; "
            "rm -f '{}'; "
            "subfolder=$(find '{}' -mindepth 1 -maxdepth 1 -type d | head -n 1); "
            "if [ -n \"$subfolder\" ]; then "
            "mv \"$subfolder\"/* '{}'; "
            "rm -rf \"$subfolder\"; "
            "fi; "
            "chmod +x '{}'",
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH],
            _executable
        );

        system(command.c_str());
        std::filesystem::remove(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE]);
    }
    else if (seven_zip_emulators.contains(core_name)) {
        command = std::format(
            "7z x -o'{}' '{}' -y; "
            "rm -f '{}'; "
            "subfolder=$(find '{}' -mindepth 1 -maxdepth 1 -type d | head -n 1); "
            "if [ -n \"$subfolder\" ]; then "
            "mv \"$subfolder\"/* '{}'; "
            "rm -rf \"$subfolder\"; "
            "fi; "
            "chmod +x '{}'",
            _directories[_directory_ids::EMULATOR_PATH],
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _downloaderDirs[_downloader_ids::DOWNLOADED_FILE],
            _directories[_directory_ids::EMULATOR_PATH],
            _directories[_directory_ids::EMULATOR_PATH],
            _executable
        );

        system(command.c_str());
        std::filesystem::remove(_downloaderDirs[_downloader_ids::DOWNLOADED_FILE]);
    }
    return true;
}
#endif

#ifdef _WIN32
#include <windows.h>

bool run_and_wait(const std::string& cmd) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);

    std::string command = cmd;

    if (!CreateProcessA(
            NULL,
            command.data(),
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi)) 
    {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to launch emulator.\n");
        return false;
    }

    // aspetta che l'emulatore venga chiuso
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}
#endif

// Compose the command to boot emulator + args. On windows system() needs powershell, because info->path formatting breaks...
// to escape and format string properly, first arg in powershell must have a dot to be considered as executable.
bool lcl_utils::lcl_core_boot(const struct retro_game_info* info) {
    std::string cmd_win{};
    std::string cmd{};
    std::string args {};
    std::string bios_arg{};

    if (_is_flatpak) {
        args = _cfg_section["FLATPAK_ARGS"].as<std::string>();
    } else {
        args = _cfg_section["ARGS"].as<std::string>();
    }
    
    if (info != NULL && info->path != NULL) {
        if (core_name == "xemu") {
            // xemu is special with spaces...
            cmd_win = std::format("powershell -c .\"'{}' {} '{}'\"", _executable, args, info->path);
        } else {
            cmd_win = std::format("powershell -c .\"'{}' '{}' '{}'\"", _executable, args, info->path);
        }
       
        cmd = std::format("'{}' '{}' '{}'", _executable, args, info->path);
    } else {
        bios_arg = _cfg_section["BIOS_ARG"].as<std::string>();
        cmd_win = std::format("\"{}\" {}", _executable, bios_arg);
        cmd = std::format("\"{}\" '{}'", _executable, bios_arg);
    }

#ifdef _WIN32
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Booting emulator with command: %s\n", cmd_win.c_str());
    
    /*
    if (system(cmd_win.c_str()) != 0) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to launch emulator.\n");
		return false;
    }
        */
    run_and_wait(cmd_win);

#elif __linux__
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Booting emulator with command: %s\n", cmd.c_str());

    if (system(cmd_flatpak.c_str()) != 0) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to launch emulator under flatpak.\n");
        return false;
    }
#endif

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

    info->library_name = core_name.c_str();
    info->library_version = "0.1a";
    info->need_fullpath = true;
    info->valid_extensions = g_emu_extensions.c_str();
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

static bool g_launched = false;

void retro_run(void)
{
    if (g_launched) {
        environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
        g_launched = false;
    }

    unsigned stride = 320;
    video_cb(frame_buf, 320, 240, stride << 2);
}

bool retro_load_game(const struct retro_game_info* info)
{
    auto core_obj = std::make_unique<lcl_utils>();

    if (!core_obj->lcl_check_config_file()) {
        return false;
    }

    // If running windows .lnk, skip url and ID setup.
    if (core_name == "windows") {
        core_obj->lcl_setup_dirs();
        core_obj->lcl_core_boot(info);
        g_launched = true;
        return true;
    }

    core_obj->lcl_setup_config_params();

    // if first boot download emulator, else check for updates
    if (core_obj->lcl_setup_dirs()) {
        core_obj->lcl_core_get();
        core_obj->lcl_core_extractor();
    } else if (core_obj->lcl_core_get()) {
        core_obj->lcl_core_extractor();
    }

    core_obj->lcl_core_boot(info);

    g_launched = true;

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