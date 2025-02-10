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

# Core installation [Windows]

`.dll` files must go in `cores` folder of retroarch

`.info` files must go in `info` folder

# Core Installation [Linux]

both `.so` and `.info` files must go in `cores` folder of retroarch

# Core Installation [macOS]

both `.dylib` and `.info` files must go in `cores` folder of retroarch

# Playlist contorller icon

I have provided a thumbnails folder with pngs inside for those console that don't have an icon, for now this is the list:

```
Xbox 360.png
```

Just copy-paste the png inside the `thumbnails` folder of retroarch

# Creating a playlist

- You can use `auto` or `manual` scan.

- In case of the Xbox 360 playlist, you need to apply manual scan. Then associate the launcher core, scan and play.
  until a rdb database is created from the respective dat file of the Xbox 360 library to support auto scan.

# NOTES

- These cores do not integrate retroarch menu when running the emulators, you will have to use the integrated emulator menus if the devs have provided them.
  Most of the menus already have controller support. Besides you must configure the standalone emulators as usual (provide BIOS files and such.)

- RPCS3 Launcher is currently unusable because when creating a playlist you can't filter by `EBOOT.BIN` which is what the emulator currently requires. Until it supports iso/chd
  or any usable format to filter the extension search, it is not possible to create a playlist. You can filter by `.bin` but that would find thousands of files,
  since RPCS3 requires you to decrypt and unpack the game files at the current state.

- Some emulators might have multiple exes with the same name (example: PPSSPPWindows.exe and PPSSPPWindows64.exe), rename/move/delete the executable that you don't need.
