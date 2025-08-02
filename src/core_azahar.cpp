#include "core_azahar.hpp"
#include "curl/curl/curl.h"
#include "json.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

// libcurl callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

core::core()
{
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

    _executable = "C:\\RetroArch - Win64\\system\\azahar\\azahar.exe";

    _asset_id = 5;

#endif

   _urls = {
        "https://api.github.com/repos/azahar-emu/azahar/releases/latest",
        "https://github.com/azahar-emu/azahar/releases/download/"
    };
}

bool core::retro_core_setup()
{
    for (auto path : _directories) {
        if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
            std::cout << "[LAUNCHER-INFO]: created folder in: " << path << std::endl;
        }
        else {
            std::cout << "[LAUNCHER-INFO]: folder already exists: " << path << std::endl;
        }
    }

    // Lookup for Emulator Executable inside Emulator folder
    if (std::filesystem::exists(_executable)) {
        std::cout << "[LAUNCHER-INFO]: Found emulator in: " << _executable << std::endl;
        return true;
    }
    else {
		std::cout << "[LAUNCHER-INFO]: Downloading emulator." << std::endl;
    }

    return false;
}

bool core::retro_core_downloader()
{
    CURL* curl = curl_easy_init(); curl_global_init(CURL_GLOBAL_DEFAULT);
    std::string jsonResponse;

    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, _urls[0].c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cout << "[LAUNCHER-ERROR]: Failed to fetch release info" << std::endl;
        return false;
    }

    json parsed;

    try {
        parsed = json::parse(jsonResponse);
    }
    catch (std::exception e) {
		std::cout << e.what() << std::endl;
        return false;
    }

    if (!parsed.contains("tag_name") || !parsed.contains("assets")) {
		std::cout << "[LAUNCHER-ERROR]: JSON missing required fields." << std::endl;
        return false;
    }

    std::string tag = parsed["tag_name"];
    const auto& assets = parsed["assets"];

    if (assets.size() <= _asset_id) {
		std::cout << "[LAUNCHER-ERROR]: Asset index out of range." << std::endl;
        return false;
    }

    std::string name = assets[_asset_id]["name"];
    std::string id =   assets[_asset_id]["id"];
    std::string url = _urls[1] + tag + "/" + name;

    std::ofstream urlOut(_downloaderDirs[0]);
    std::ofstream idOut(_downloaderDirs[1]);

    if (!urlOut || !idOut) {
		std::cout << "[LAUNCHER-ERROR]: Cannot write metadata files." << std::endl;
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
    if (!curl) return false;

    FILE* outFile = fopen(outputFile.c_str(), "wb");
    if (!outFile) {
		std::cout << "[LAUNCHER-ERROR]: Failed to open output file." << std::endl;
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(outFile);

    if (res != CURLE_OK) {
        std::cout << "[LAUNCHER-ERROR]: Failed to download emulator." << std::endl;
       return false;
    }

	std::cout << "[LAUNCHER-INFO]: Downloaded emulator to: " << outputFile << std::endl;
    return true;

}
