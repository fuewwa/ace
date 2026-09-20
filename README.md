![Title](assets/images/title.png)

---

![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)
![Platform](https://img.shields.io/badge/platform-X11-lightgrey.svg)
![PRs welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)
![build](https://github.com/KURWAss/ace/actions/workflows/build.yml/badge.svg)
[![Distros](https://img.shields.io/badge/Distributions-Arch_Linux,_Debian-green.svg)](https://shields.io/)

A minimal floating window manager for X11, written in C++. One of biggest plus: ace uses only ~5 RAM megabytes

## Keybinds

- Maps new windows as soon as they are created
- Move a window: `Alt` + left mouse button + drag
- Resize a window: `Alt` + right mouse button + drag
- Quit the window manager: `Super` + `Ctrl` + `Q`
- Close a window: `Super` + `Q`
- Open the application launcher: `Super` + `D`
- Open a terminal: `Super` + `Space`
- Switch workspace: `Super` + `0`-`9`
- Make fullscreen: `Super` + `F`

## Project layout

```
ace/
├── include/           header files
│   ├── window_manager.h
│   ├── ewmh.h
│   ├── process.h
│   ├── workspaces.h
│   └── config.h
├── src/                source files
│   ├── main.cpp
│   ├── window_manager.cpp
│   ├── ewmh.cpp
│   ├── process.cpp
│   ├── workspaces.cpp
│   └── config.cpp
├── Makefile
├── README.md
├── CONTRIBUTING.md
└── ... (Other files)
```

## System-wide installation
 
If you want to select `ace` as a session from your login manager
(LightDM, GDM, SDDM, etc.), use the commands in Makefile.
 
`make install` copies the `ace` binary to `/usr/bin` and
`assets/ace.desktop` to `/usr/share/xsessions`, so it shows up in the
session picker on the login screen:
 
```
sudo make clean install
```
 
`make uninstall` removes both of these:
 
```
sudo make uninstall
```
 
Both commands require root, since they write to `/usr/bin` and
`/usr/share/xsessions`.

## Apps Dependencies
 
ace itself doesn't launch anything on its own — it relies on external
programs configured by you:
 
- a terminal emulator, defaulting to [alacritty](https://alacritty.org/)
- an application launcher, defaulting to [rofi](https://github.com/davatorium/rofi)

On first run, ace creates a config file at `~/.config/ace/config` (if it
doesn't already exist)
 
You can change `alacritty` (or any other program) to any terminal emulator installed on your
system (e.g. `xterm`, `kitty`, `foot`) — ace will launch whatever you put
there when you press `Super` + `Space`.

## Dependencies

- g++ with C++17 support
- libX11 (headers and library)

On Arch Linux:

```
sudo pacman -S libx11 xorg-server-xephyr
```

On Debian based:

```
sudo apt install libx11-dev xserver-xephyr
```

## Building

```
make
```

The `ace` binary will be produced in the project root. You can write `make help` to see Makefile command list

## Running

For testing without leaving your current session, use Xephyr:

```
Xephyr :1 -screen 1280x800 &
DISPLAY=:1 ./ace
```
Alternatively, just run:

```
make run
```

This checks that Xephyr is installed, builds `ace` if needed, then starts
Xephyr and launches `ace` on `DISPLAY=:1`.

Then, in that same `DISPLAY=:1`, you can launch any X11 application, e.g.:
```
DISPLAY=:1 xterm
```
To use `ace` as your main window manager, add this to `~/.xinitrc`:
```
exec /path/to/ace
```
and start your session with `startx`. Or select in your greeter

## Screenshots

![Screen1](assets/images/screenshot1.png)
![Screen2](assets/images/screenshot2.png)
![Screen3](assets/images/screenshot3.png)

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3).
See the `LICENSE` file for the full text.
