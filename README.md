# libretro-core-launchers
Custom launchers to allow the execution of emulators instead of integrated cores.

# Setup

The cores search automatically in the default path of retroarch.

- Windows: `C:\RetroArch-Win64\system`

- Linux: `~/.config/retroarch/system`

- macOS: TODO

for each emulator you must create a folder in `system` like this:

```
duckstation
pcsx2
rpcs3
PPSSPP
xemu
xenia_canary
mGBA
lime3ds
melonDS
```

then download the emulator you want and drag & drop it in the respective created folder.

# Core files installation

`.dll` or `.so` files must go in `cores` folder of retroarch

`.info` files must go in `info` folder

The cores are programmed to search in both windows/linux the binary to execute, so no need to worry about the executable/binary name or extensions.


# Playlist contorller icon

I have provided a thumbnails folder with pngs inside for those console that don't have an icon, for nor this is the list:

```
Xbox 360.png
```

Just copy-paste the png inside the `Thumbnails` folder of retroarch

# Creating a playlist

- Use manual scan, if the system name it's not available then choose `custom` and type your custom name.

- In case of the Xbox 360 playlist, the core has the associated icon in the `.info` file

- add your rom extensions

- Associate the launcher core

- Scan and play.

# NOTES

- These cores do not integrate retroarch menu when running the emulators, you will have to use the integrated emulator menus if the devs have provided them.
  Most of the menus already have controller support. Besides you must configure the emulator as usual (provde BIOS files, HDD image and such)

- RPCS3 Launcher is currently unusable because when creating a playlist you can't filter by `EBOOT.BIN` which is what the emulator currently requires. Until it supports iso/chd
  or any usable format to filter the extension search, it is not possible to create a playlist.
