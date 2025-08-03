#include <vector>
#include <string>

class core
{
public:
	core();

	bool retro_core_setup();
	bool retro_core_downloader();
	bool retro_core_updater();
    bool retro_core_extractor();

private:
    std::vector<std::string> _directories;
    std::vector<std::string> _downloaderDirs;    
    std::vector<std::string> _urls;
	std::string _executable;
	int _asset_id;
	//const int _asset_id_linux = 9;
	//const int _asset_id_macos = 2;
};