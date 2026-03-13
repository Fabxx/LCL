#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "curl/curl.h"
#include "libretro.h"


class core
{
public:
	core();

	
	bool retro_core_setup_dirs();
	bool retro_core_setup_urls_ids();

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

	bool _is_flatpak;

	// IDs for the assets in the GitHub release section
	int ASSET_ID;
	
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