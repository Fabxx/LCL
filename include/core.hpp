#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "curl/curl.h"
#include "libretro.h"
#include "nlohmann/json.hpp"

class core
{
public:
	core();

	bool retro_core_setup();
	bool retro_core_get();
	bool retro_core_extractor();
	bool retro_core_updater();
	bool retro_core_boot(const struct retro_game_info* info);

	bool build_download_url(CURL* curl, CURLcode& res);
	bool download_asset(CURL* curl, CURLcode& res, std::string& url);


private:
	std::vector<std::string> _directories;
	std::vector<std::string> _downloaderDirs;
	std::vector<std::string> _urls;

	std::string _executable;
	std::string _tag;
	std::string _current_version;
	std::string _new_version;

	// using path to not worry about separators
	std::filesystem::path _base_path;

	int _asset_id;
	int _url_asset_id;

	// IDs for the assets in the GitHub release section
	enum _asset_ids {
		
		AZAHAR_WIN = 4,
		AZAHAR_LINUX = 9,
		AZAHAR_MACOS = 2,

		DUCKSTATION_WIN = 9,
		DUCKSTATION_LINUX = 13,
		DUCKSTATION_MACOS = 3,
		
		MGBA_WIN = 16,
		MGBA_LINUX = 2,
		MGBA_MACOS = 3,
		
		MELONDS_WIN = 5,
		MELONDS_LINUX = 2,
		MELONDS_MACOS = 0,
		
		PCSX2_WIN = 5,
		PCSX2_LINUX = 0,
		PCSX2_MACOS = 2,
		
		XEMU_WIN = 11,
		XEMU_LINUX = 6,
		XEMU_MACOS = 2,
		
		XENIA_WIN = 0,
		//XENIA_LINUX = 0 not available yet.
	};

   enum _directory_ids {
       EMULATOR_PATH,
       THUMBNAILS_PATH,
       BOXARTS_PATH,
       SNAPS_PATH,
       TITLES_PATH
    };

   enum _downloader_ids {
        URL_FILE,
        CURRENT_VERSION_FILE,
        NEW_VERSION_FILE,
        DOWNLOADED_FILE
    };

   enum _url_ids {
        LATEST_RELEASE_URL,
        BASE_URL,
        DOWNLOAD_URL
    };
};