#pragma once

class base_core {
public:
	virtual bool retro_core_setup() = 0;
	virtual bool retro_core_downloader() = 0;
	//virtual bool retro_core_updater() = 0;
	//virtual bool retro_core_extractor() = 0;
};