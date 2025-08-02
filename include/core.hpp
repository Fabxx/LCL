#include "base_core.hpp"
#include "libretro.h"
#include <vector>
#include <string>

class core : public base_core
{
public:
	core();
	bool retro_core_setup() override;
	bool retro_core_downloader() override;

private:
    std::vector<std::string> _directories;
    std::vector<std::string> _downloaderDirs;    
    std::vector<std::string> _urls;
	std::string _executable;
	int _asset_id = 5;
};