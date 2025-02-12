# libretro-core-launchers
Custom launchers to allow the execution of emulators instead of integrated cores.

Initially based on this repo: https://github.com/new-penguin/libretro-xemu-launcher-antimicrox

The code has been refactored to add support to windows/Linux in a more generic way.

# Setup

The cores search automatically in the default path of retroarch.

- Windows: `C:\RetroArch-Win64\system`

- Linux: `~/.config/retroarch/system`

- macOS: `~/Library/Application Support/RetroArch/system`

Running a core will generate a folder with the name of the emulator to use.

then download the emulator you want and drag & drop it in the respective created folder.

# Core installation

Windows:
- `.dll` files must go in `cores` folder of retroarch
-  `.info` files must go in `info` folder

Linux:
- `.so` and `.info` files must go in `cores` folder of retroarch

MacOS:
- `.dylib` and `.info` files must go in `cores` folder of retroarch


# Playlist custom controller icon

I have provided a thumbnails folder with pngs inside for those console that don't have an icon, for now this is the list:

```
Xbox 360.png
```

Just copy-paste the png inside the `thumbnails` folder of retroarch

# Creating a playlist

- You can use `auto` or `manual` scan. There is support for local covers, or downloaded box arts thanks to the rdb databases.

  NOTE: Covers must be in `.png` and placed in `thumbnails/system name` folder of retroarch. Inside you will have:

  - `Named_Boxarts`
  - `Named_Snaps`
  - `Named_Titles`
 
  System name can be `Sony - Playstation` etc.

If you don't have these folders you can create them.

NOTE: Game names and covers must NOT have special characters like `-` `&` `'`

- In case of the Xbox 360 playlist, you need to apply manual scan. Then associate the launcher core, scan and play.
  until a rdb database is created from the respective dat file of the Xbox 360 library to support auto scan.

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

- Some emulators might have multiple exes with the same name (example: PPSSPPWindows.exe and PPSSPPWindows64.exe),
  rename/move/delete the executable that you don't need.

# Dev notes

Retroarch initializes `info` structure and `info->path` member to get the game to load. If no game is given structure is NULL, if a game is given,
`info->path` can be passed as argument, both `info` and `info->path` must NOT be NULL.

If no games are expected don't pass info->path, just string arguments.
