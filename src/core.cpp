#include "core.hpp"
#include <iostream>

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
    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Core name: %s\n", core_name.c_str());

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

#ifdef _WIN32

    if (core_name == "azahar") {
        _asset_id = _asset_ids::AZAHAR_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "azahar.zip").string());
        _executable = (_base_path / "system" / core_name / "azahar.exe").string();
    }

    if (core_name == "duckstation") {
        _asset_id = _asset_ids::DUCKSTATION_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "DuckStation-x64.zip").string());
        _executable = (_base_path / "system" / core_name / "duckstation-qt-x64-ReleaseLTCG.exe").string();
    }

    if (core_name == "mgba") {
        _asset_id = _asset_ids::MGBA_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "mGBA.7z").string());
        _executable = (_base_path / "system" / core_name / "mGBA.exe").string();
    }
    if (core_name == "melonds") {
        _asset_id = _asset_ids::MELONDS_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "melonDS.zip").string());
        _executable = (_base_path / "system" / core_name / "melonDS.exe").string();
    }
    if (core_name == "pcsx2") {
        _asset_id = _asset_ids::PCSX2_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "pcsx2.7z").string());
        _executable = (_base_path / "system" / core_name / "pcsx2-qt.exe").string();
    }
    if (core_name == "xemu") {
        _asset_id = _asset_ids::XEMU_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "xemu.zip").string());
        _executable = (_base_path / "system" / core_name / "xemu.exe").string();
    }
    if (core_name == "xenia") {
        _asset_id = _asset_ids::XENIA_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "xenia_canary_netplay.zip").string());
        _executable = (_base_path / "system" / core_name / "xenia_canary_netplay.exe").string();
    }

#elif __linux__
    
    if (core_name == "azahar") {
        _asset_id = _asset_ids::AZAHAR_LINUX;
        _executable = (_base_path / "system" / core_name / "azahar.AppImage").string();
        _downloaderDirs.push_back(_executable);
    }

    if (core_name == "duckstation") {
        _asset_id = _asset_ids::DUCKSTATION_LINUX;
        _executable = (_base_path / "system" / core_name / "duckstation.AppImage").string();
        _downloaderDirs.push_back(_executable);
    }

    if (core_name == "mgba") {
        _asset_id = _asset_ids::MGBA_LINUX;
        _executable = (_base_path / "system" / core_name / "mGBA.AppImage").string();
        _downloaderDirs.push_back(_executable);
    }
    
    if (core_name == "melonds") {
        _asset_id = _asset_ids::MELONDS_LINUX;
        _executable = (_base_path / "system" / core_name / "melonDS-x86_64.AppImage").string();
        _downloaderDirs.push_back(_executable + ".zip");
    }
    
    if (core_name == "pcsx2") {
        _asset_id = _asset_ids::PCSX2_LINUX;
        _executable = (_base_path / "system" / core_name / "pcsx2.AppImage").string();
        _downloaderDirs.push_back(_executable);
    }
    
    if (core_name == "xemu") {
        _asset_id = _asset_ids::XEMU_LINUX;
        _executable = (_base_path / "system" / core_name / "xemu.AppImage").string();
        _downloaderDirs.push_back(_executable);
    }
    
    if (core_name == "xenia") {
        _asset_id = _asset_ids::XENIA_WIN;
        _downloaderDirs.push_back((_base_path / "system" / core_name / "xenia_canary_netplay.zip").string());
        _executable = (_base_path / "system" / core_name / "xenia_canary_netplay.exe").string();
    }
#endif

    _url_asset_id = 0;

    if (core_name == "azahar") {
        _urls = {
            "https://api.github.com/repos/azahar-emu/azahar/releases/latest",
            "https://github.com/azahar-emu/azahar/releases/download"
        };
    }
    else if (core_name == "duckstation") {
        _urls = {
          "https://api.github.com/repos/stenzek/duckstation/releases/latest",
          "https://github.com/stenzek/duckstation/releases/download"
        };
    }
    else if (core_name == "mgba") {
        _urls = {
          "https://api.github.com/repos/mGBA-emu/mGBA/releases/latest",
          "https://github.com/mGBA-emu/mGBA/releases/download"
        };
    }
    else if (core_name == "melonds") {
        _urls = {
          "https://api.github.com/repos/melonDS-emu/melonDS/releases/latest",
          "https://github.com/melonDS-emu/melonDS/releases/download"
        };
    } else if (core_name == "pcsx2") {
        _urls = {
         "https://api.github.com/repos/PCSX2/pcsx2/releases/latest",
         "https://github.com/PCSX2/pcsx2/releases/download"
        };
    }
    else if (core_name == "xemu") {
        _urls = {
         "https://api.github.com/repos/xemu-project/xemu/releases/latest",
         "https://github.com/xemu-project/xemu/releases/download"
        };
    }
    else if (core_name == "xenia") {
        _urls = {
        "https://api.github.com/repos/AdrianCassar/xenia-canary/releases/latest",
        "https://github.com/AdrianCassar/xenia-canary/releases/download",
        };
    }
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
        return false;
    }

    log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Downloading emulator.\n");
    return true;
}

bool core::build_download_url(CURL* curl, CURLcode& res)
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

    if (_asset_id >= assets.size()) {
        log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Asset ID %d out of range.\n", _asset_id);
        return false;
    }

    std::string name = assets[_asset_id]["name"];
    _url_asset_id = assets[_asset_id]["id"];
    _current_version = std::to_string(_url_asset_id);
    _new_version = std::to_string(_url_asset_id);
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
        return false;
    }

    return true;
}

bool core::download_asset(CURL *curl, CURLcode& res, std::string &url)
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

bool core::retro_core_get()
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    if (!build_download_url(curl, res)) {
		log_cb(RETRO_LOG_ERROR, "[LAUNCHER-ERROR] Failed to set URL.\n");
        return false;
    }

    bool firstBoot = !std::filesystem::exists(_executable);

    if (firstBoot) {
        log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] First boot detected, downloading emulator...\n");

        if (!download_asset(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
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

        if (!download_asset(curl, res, _urls[_url_ids::DOWNLOAD_URL])) {
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
bool core::retro_core_extractor()
{
    const std::filesystem::path archive_path = _downloaderDirs[_downloader_ids::DOWNLOADED_FILE];
    const std::string ext = archive_path.extension().string();
    std::string command{};

    const std::unordered_set<std::string> supported_exts = { ".zip", ".7z" };
    const std::unordered_set<std::string> zip_emulators = { "azahar", "duckstation", "melonds", "xemu", "xenia" };
    const std::unordered_set<std::string> seven_zip_emulators = { "mgba", "pcsx2" };

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
            "Move-Item -Path \\\"$($subfolder.FullName)\\\\*\\\" -Destination '{}' -Force; "
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
		log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO]: Extracting emulator from 7z archive.\n");
        
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
            "Move-Item -Path \\\"$($subfolder.FullName)\\\\*\\\" -Destination '{}' -Force; "
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
bool core::retro_core_extractor()
{
    const std::filesystem::path archive_path = _downloaderDirs[_downloader_ids::DOWNLOADED_FILE];
    const std::string ext = archive_path.extension().string();
    std::string command{};

    const std::unordered_set<std::string> supported_exts = { ".zip", ".7z", ".tar"};
    const std::unordered_set<std::string> zip_emulators = { "azahar", "duckstation", "melonds", "xemu", "xenia" };
    const std::unordered_set<std::string> seven_zip_emulators = { "mgba", "pcsx2" };

    if (ext == ".AppImage") {
        command = std::format("chmod +x '{}'", _executable);
        system(command.c_str());
    }

    // Caso archivi
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


// Compose the command to boot emulator + args. Quote arguments that can have spaces.
bool core::retro_core_boot(const struct retro_game_info* info)
{
    std::string command{};
    std::string args{};

    if (core_name == "azahar") {
        if (info != NULL && info->path != NULL) {
            command = std::format("\"{}\" \"{}\"", _executable, info->path);
        }
        else {
            command = std::format("\"{}\"", _executable);
        }
    }
    else if (core_name == "duckstation") {
        if (info != NULL && info->path != NULL) {
            args = "-fullscreen";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            args = "-fullscreen -bios";
            command = std::format("\"{}\" {}", _executable, args);
        }
    }
    else if (core_name == "mgba") {
        if (info != NULL && info->path != NULL) {
            args = "-f";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            command = std::format("\"{}\"", _executable);
        }
    }
    else if (core_name == "melonds") {
        if (info != NULL && info->path != NULL) {
            args = "-f";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            command = std::format("\"{}\"", _executable);
        }
    }
    else if (core_name == "pcsx2") {
        if (info != NULL && info->path != NULL) {
            args = "-fullscreen";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            args = "-fullscreen -bios";
            command = std::format("\"{}\" {}", _executable, args);
        }
    }
    else if (core_name == "xemu") {
        if (info != NULL && info->path != NULL) {
            args = "-full-screen -dvd_path";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            args = "-full-screen";
            command = std::format("\"{}\" {}", _executable, args);
        }
    }
    else if (core_name == "xenia") {
        if (info != NULL && info->path != NULL) {
            args = "--fullscreen=true";
            command = std::format("\"{}\" {} \"{}\"", _executable, args, info->path);
        }
        else {
            command = std::format("\"{}\"", _executable);
        }
    }

	log_cb(RETRO_LOG_INFO, "[LAUNCHER-INFO] Booting emulator with command: %s\n", command.c_str());
	system(command.c_str());
    return true;
}

//TODO
bool core::retro_core_updater()
{
    std::vector<std::string> core_urls = {
    };

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
    
    if (core_name == "azahar") {
        info->library_name = "Azahar";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "3ds|3dsx|elf|axf|cci|cxi|app";
    }
    else if (core_name == "duckstation") {
        info->library_name = "Duckstation";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "cue|img|ecm|chd";
    }
    else if (core_name == "mgba") {
        info->library_name = "mGBA";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "gba|gbc|gc";
    }
    else if (core_name == "melonds") {
        info->library_name = "melonDS";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "nds|ids|dsi";
    }
    else if (core_name == "pcsx2") {
        info->library_name = "PCSX2";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "iso|chd|elf|ciso|cso|bin|cue|mdf|nrg|dump|gz|img|m3u";
    }
    else if (core_name == "xemu") {
        info->library_name = "Xemu";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "iso";
    }
    else if (core_name == "xenia") {
        info->library_name = "Xenia";
        info->library_version = "0.1a";
        info->need_fullpath = true;
        info->valid_extensions = "iso|xex|zar";
    }
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

    // if first boot download emulator, else check for updates
    if (core.retro_core_setup()) {
        core.retro_core_get();
        core.retro_core_extractor();
    } else if (core.retro_core_get()) {
        core.retro_core_extractor();
    }

    core.retro_core_boot(info);

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