# libretro-core-launchers
Custom launchers to allow the execution of emulators instead of integrated cores.

Initially based on this repo: https://github.com/new-penguin/libretro-xemu-launcher-antimicrox

The code has been refactored to add support to windows/Linux in a more generic way.

# Default Paths

- Windows: `C:\RetroArch-Win64\system`

- Linux: `~/.config/retroarch/system`

- macOS: `~/Library/Application Support/RetroArch/system`

# Usage

The core does the following:

- Download the emulator files if it doesn't exist
- Setup thumbnail folders for boxarts, snaps and title images for the selected system
- Create a `bios` folder under `retroarch/system/system name`
- Reboots retroarch to let the user run the core, with BIOS (if supported) or with a game from playlist


# Core installation

Windows:
- `.dll` files must go in `cores` folder of retroarch
-  `.info` files must go in `info` folder

Linux:
- `.so` and `.info` files must go in `cores` folder of retroarch

MacOS:
- `.dylib` and `.info` files must go in `cores` folder of retroarch


# Playlist custom controller icon

for those console that don't have an icon, copy-paste the png from `thumbnails` inside the `thumbnails` folder of retroarch

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

- Xenia canary requires wine and winetricks in order to run under `Linux`


# Dev notes

Retroarch initializes `info` structure and `info->path` member to get the game to load. If no game is given structure is NULL, if a game is given,
`info->path` can be passed as argument, both `info` and `info->path` must NOT be NULL.

If no games are expected don't pass info->path, just string arguments.
