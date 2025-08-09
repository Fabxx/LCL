# DISCLAIMER
The cores DO NOT integrate any emulator code. All they do is to download the official emulators from github sources, and then allow the user to use those from retroarch UI.

# Windows Only Setup

- To allow the `Invoke Web-Request` command, internet explorer must be run at least once, and configure it with default options.
 

# Building

- Install `Cmake`

- open terminal and paste this: `git clone "https://github.com/Fabxx/LCL"`

- `\.build_all.ps1` if on windows, `./build_all.sh` if on linux.

# Core installation

- Windows:
  - `.dll` files in `cores` folder of retroarch
  - `.info` files in `info` folder of retroarch

- Linux:
  - `.so` and `.info` files in `cores` folder of retroarch
  
 
# Core behavior

- Fetch (first boot):
   - invoke a web request on github API, to fetch: `tag name`, `url`, `url id`, `asset id`.
   - Compose the final URL.
   - download and extract the detected archive
   - export url and IDs in txt files for updater.

- Update:
   - If the binary is detected, contact the github API to retreive the new URL id.
   - If URLs/IDs are different, a new download on the newest URL will be invoked.
   - The current version will be overwritten for next comparisons.

- Thumnail Setup
   -  Covers must be in `.png` and placed in `thumbnails/system name` folder of retroarch. Inside you will have:
      - `Named_Boxarts`
      - `Named_Snaps`
      - `Named_Titles`
        
  - Game names and covers must NOT have special characters like dash, ampersand, apostrophe etc
  - Game and cover names must be identical if using manual scan.
  - You can get some thumbnails from here (boxarts): https://shorturl.at/HifC4
    - Drag & drop the thumbnails folder inside the retroarch installation folder.

- Boot
   - If choosing `Run core` the emulator will boot without the `info->path` which contains the path to the game.
     and if supported it will run with the given BIOS.
   - If running a game from a playlist `info->path` is given as a argument to the emulator executable.



# Creating a playlist [Directory Scan]

- Retroarch will automatically detect the supported ROMs and with the rdb databases it will manage to download the corresponding
  covers.

# Creating a Playlist [Manual Scan]

- In manual scan you have to give the system name for those that are not included in retroarch databases.
- The system will detect your local covers/artworks in this case.

# BIOS Notes
  - pcsx2 will run the configured BIOS through `run core` with `-bios` flag, but it must be configured first in pcsx2 GUI
    
  - Duckstation behaves like pcsx2 for BIOS
    
  - mGBA allows to select only the BIOS file with `--bios/-b` flags, but you can run it only from GUI
 
  - melonDS behaves like mGBA
 
  - RPCS3 Requires you to install the firmware and then boot the XMB from GUI
    
  - Xemu runs the bios by default if no game is selected. BIOS must be configured from GUI
    
  - If no BIOS is configured each emulator will just open the GUI.

  - PPSSPP, xenia_canary, lime3DS do not provide a BIOS functionality yet.


# Dev notes

- Retroarch initializes `info` structure and `info->path` member to get the game to load. If no game is given structure is NULL, if a game is given,
  `info->path` can be passed as argument, both `info` and `info->path` must NOT be NULL.

# Reporting Issues
To check the error of a core, launch retroarch from terminal (linux) or enable console logging from windows.

Launchers have their own log messages which can be `[LAUNCHER-INFO]` or `[LAUNCHER-ERROR]`

You should create an issue with these messages, it will help to understand the issue.


# Showcase

Example when running xemu (BIOS Configured in xemu itself before launch)

https://github.com/user-attachments/assets/eec0f0eb-acb3-4790-906f-57cd18cc122d




