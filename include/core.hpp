#pragma once

#include <vector>
#include <string>

#include "curl/curl.h"
#include "libretro.h"
#include "nlohmann/json.hpp"

// Core name that is declared from CMake, then passed to code.
#ifndef CORE
#error "CORE is not defined!"
#endif

#ifndef SYSTEM_NAME
#error "SYSTEM is not defined!"
#endif

/*
 * IDs that identify the assets under the "Release" section in the JSON response from GitHub
 * Numbers follow the display order of the archives in the release section.
 */
#define AZAHAR_WIN 4
#define AZAHAR_LINUX 9
#define AZAHAR_MACOS 2

#define DUCKSTATION_WIN 9
#define DUCKSTATION_LINUX 13
#define DUCKSTATION_MACOS 3

#define MGBA_WIN 16
#define MGBA_LINUX 2
#define MGBA_MACOS 3

#define MELONDS_WIN 5
#define MELONDS_LINUX 2
#define MELONDS_MACOS 0

#define PCSX2_WIN 5
#define PCSX2_LINUX 0
#define PCSX2_MACOS 2

#define XEMU_WIN 11
#define XEMU_LINUX 6
#define XEMU_MACOS 2

#define XENIA_WIN 0

class core
{
public:
	core();

	bool check_retroarch_path();
	bool retro_core_setup();
	bool retro_core_get();
	bool retro_core_extractor();
	bool retro_core_boot(struct retro_system_info* info);

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

	enum _asset_ids
	{
#ifdef CORE == "azahar"

		WINDOWS = AZAHAR_WIN,
		LINUX = AZAHAR_LINUX,
		MACOS = AZAHAR_MACOS

#elif CORE == "duckstation"

		WINDOWS = DUCKSTATION_WIN,
		LINUX = DUCKSTATION_LINUX,
		MACOS = DUCKSTATION_MACOS

#elif CORE == "mgba"

		WINDOWS = MGBA_WIN,
		LINUX = MGBA_LINUX,
		MACOS = MGBA_MACOS

#elif CORE == "melonds"
		
		WINDOWS = MELONDS_WIN,
		LINUX = MELONDS_LINUX,
		MACOS = MELONDS_MACOS

#elif CORE == "pcsx2"
		WINDOWS = PCSX2_WIN,
		LINUX = PCSX2_LINUX,
		MACOS = PCSX2_MACOS

#elif CORE == "xemu"

		WINDOWS = XEMU_WIN,
		LINUX = XEMU_LINUX,
		MACOS = XEMU_MACOS

#elif CORE == "xenia"

		WINDOWS = XENIA_WIN
#endif
	};

// TODO: RPCS3, Ryujinx, Xenon
};