#include <vector>
#include <string>

#include "curl/curl.h"
#include "libretro.h"
#include "nlohmann/json.hpp"

class core
{
public:
	core();

	bool retro_core_setup();
	bool retro_core_downloader();
	bool retro_core_extractor();
	bool retro_core_boot(struct retro_system_info* info);

	bool set_url(CURL* curl, CURLcode& res);
	bool download(CURL *curl, CURLcode& res, std::string& url);
    

private:
    std::vector<std::string> _directories;
    std::vector<std::string> _downloaderDirs;    
    std::vector<std::string> _urls;
	
	std::string _executable;
	std::string _tag;

	int _asset_id;
	int _url_asset_id;


	enum _directory_ids
	{
		EMULATOR_PATH,
		THUMBNAILS_PATH,
		BOXARTS_PATH,
		SNAPS_PATH,
		TITLES_PATH
	};

	enum _downloader_ids
	{
		URL_FILE,
		CURRENT_VERSION_FILE,
		NEW_VERSION_FILE,
		DOWNLOADED_FILE
	};

	enum _url_ids
	{
		LATEST_RELEASE_URL,
		BASE_URL,
		DOWNLOAD_URL
	};

	//const int _asset_id_linux = 9;
	//const int _asset_id_macos = 2;
};