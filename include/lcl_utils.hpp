#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <inicpp.h>

#include "curl/curl.h"

class lcl_utils {
public:
	lcl_utils();

	bool lcl_check_config_file();
	bool lcl_check_flatpak();
	bool lcl_load_config_file();
	bool lcl_setup_dirs();
	bool lcl_setup_config_params();

	bool lcl_core_get();
	bool lcl_core_extractor();
	bool lcl_core_updater();
	bool lcl_core_boot(const struct retro_game_info* info);
	bool lcl_build_download_url(CURL* curl, CURLcode& res);
	bool lcl_download_asset(CURL* curl, CURLcode& res, std::string& url);

	bool lcl_get_config_status();

private:
	std::vector<std::string> _directories;
	std::vector<std::string> _downloaderDirs;
	std::vector<std::string> _urls;

	std::string _executable;
	std::string _tag;
	std::string _current_version;
	std::string _new_version;
	std::string _config_path;
	std::string _emu_extensions;
	std::string _search_token;

	// using path to not worry about separators
	std::filesystem::path _base_path;

	ini::IniFile _cfg;
	ini::IniSection _cfg_section;
	
	int _url_asset_id;

	bool _is_flatpak;
	
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