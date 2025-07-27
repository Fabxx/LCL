# libretro-core-launchers
Custom launchers to allow the execution of emulators instead of integrated cores.

Initially based on this repo: https://github.com/new-penguin/libretro-xemu-launcher-antimicrox

The code has been refactored to add support to windows/Linux in a more generic way.

# DISCLAIMER
The cores DO NOT integrate any emulator code. All they do is to download the official emulators from github sources, and then allow the user to use those from retroarch UI.

# Why?
I find the XMB very comfortable. Unfortunately some emulators are not supported by retroarch, or some are supported, but they need some time to 
catch up with the standalone releases, which is understandable considering the retroArch ecosystem.

So i've made these cores to facilitate the usage of emulators, and to use the up-to-date releases.

# Default Paths

- Windows: `C:\RetroArch-Win64\system`

- Linux: `~/.config/retroarch/system`

- macOS: `~/Library/Application Support/RetroArch/system`


# Dependencies

- `jq` JSON parser package for Linux
- `bash` interpreter for Linux
- `powershell` for Windows
  
# Core behavior

1) Fetch (first boot):
   - invoke a web request on github API, to fetch: `tag name`, `url`, `url id`, `asset name`.
   - Compose the final URL by concatenating base url and the retreived details.
   - download and extract the detected asset archive, and depending on the extension use the appropriate
     extraction command, which can be: `tar`, `unzip`, `Expand-Archive`, `Expand-7zip`
   - export url and IDs in txt files for updater.

2) Update:
   - If the binary is detected, contact the github API to retreive the new URL or URL id, and compare them.
   - If URLs/IDs are different, a new download on the newest URL will be invoked.
   - The current version will be overwritten for next comparisons.
   - In case of RPCS3 For windows and PPSSPP, the URLs are directly retreived from the website HTML, which contain directly
     the latest url.
   - In case of mGBA for macOS, the `.dmg` image is downloaded, then `hdiutil` mounts it to a `tmp_dir` with user privileges,
     and then the needed content is copied.

3) Thumnail Setup
   - The cores automatically create the directories needed for artworks. See below for more info.
4) Boot
   - If choosing `Run core` the emulator will boot without the `info->path` which contains the path to the game.
     and if supported it will run with the given BIOS.
   - If running a game from a playlist `info->path` is given as a argument to the emulator executable.

# Core installation

Windows:
- `.dll` files must go in `cores` folder of retroarch
-  `.info` files must go in `info` folder

Linux:
- `.so` and `.info` files must go in `cores` folder of retroarch

MacOS:
- `.dylib` and `.info` files must go in `cores` folder of retroarch


# Playlist controller icons

Theme icons are compatible with the cores, since they match the system name of the pngs.

You can check the names inside `retroarch/assets/driverName/themeName`

In my case it would be `retroarch/assets/xmb/systematic`

# Creating a playlist [Directory Scan]

- Retroarch will automatically detect the supported ROMs and with the rdb databases it will manage to download the corresponding
  covers.

# Creating a Playlist [Manual Scan]

- In manual scan you have to also give the system name, if no extensions are given they will be detected from the core.
- The system will detect your local covers/artworks in this case.

NOTE: Covers must be in `.png` and placed in `thumbnails/system name` folder of retroarch. Inside you will have:

  - `Named_Boxarts`
  - `Named_Snaps`
  - `Named_Titles`


- Game names and covers must NOT have special characters like dash, ampersand, apostrophe etc

- Game and cover names must be identical if using manual scan.


# Thumbnails

Link: https://shorturl.at/HifC4

- Drag & drop the thumbnails folder inside the retroarch installation folder.


# BIOS Notes
  - pcsx2 will run the configured BIOS through `run core` with `-bios` flag, but it must be configured first in pcsx2 GUI
    
  - Duckstation behaves like pcsx2 for BIOS
    
  - mGBA allows to select only the BIOS file with `--bios/-b` flags, but you can run it only from GUI
 
  - melonDS behaves like mGBA
 
  - RPCS3 Requires you to install the firmware and then boot the XMB from GUI
    
  - Xemu runs the bios by default if no game is selected. BIOS must be configured from GUI
    
  - If no BIOS is configured each emulator will just open the GUI.

  - PPSSPP, xenia_canary, lime3DS do not provide a BIOS functionality yet.

# NOTES

- you must configure the standalone emulators from their GUI (provide BIOS files, video settings and such.)

- RPCS3 Launcher is currently unusable. You can filter by `.bin` but that would find thousands of files,
  since RPCS3 requires you to decrypt and unpack the game files at the current state, and launch from `EBOOT.BIN`

- Xenia canary requires wine and winetricks in order to run under `Linux/macOS`. 
  
  This is handled by the core to check if `wine` or `winetricks` commands fail. 
  
  If they do, install them. on `macOS` you need `homebrew` in order to install wine and winetricks.


# Dev notes

Retroarch initializes `info` structure and `info->path` member to get the game to load. If no game is given structure is NULL, if a game is given,
`info->path` can be passed as argument, both `info` and `info->path` must NOT be NULL.

# Reporting Issues
To check the error of a core, launch retroarch from terminal (linux) or enable console logging from windows.

Launchers have their own log messages which can be `[LAUNCHER-INFO]` or `[LAUNCHER-ERROR]`

You should create a issue with these messages, it will help to understand the issue.


# Showcase

Example when running xemu (BIOS Configured in xemu itself before launch)

https://github.com/user-attachments/assets/eec0f0eb-acb3-4790-906f-57cd18cc122d


