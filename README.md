# Hyprload
A (WIP) minimal [Hyprland](https://github.com/hyprwm/Hyprland) plugin manager

# Features
- [x] Loading plugins
- [x] Reloading plugins
- [ ] Installing and updating plugins automatically
    - [ ] A unified plugin manifest format
    - [ ] Installing from git
    - [ ] Installing from a url

# Installing
Since Hyprland plugins don't have ABI guarantees, you *should* download the Hyprland source and compile it if you plan to use plugins.
This ensures the compiler version is the same between the Hyprland build you're running, and the plugins you are using.

The guide on compiling and installing Hyprland manually is on the [wiki](http://wiki.hyprland.org/Getting-Started/Installation/#manual-manual-build)

## Steps
1. Export the `HYPRLAND_HEADERS` variable to point to the root directory of the Hyprland repo
    - `export HYPRLAND_HEADERS="$HOME/repos/Hyprland"`
2. Install `hyprload`
    - `make install`
3. Add this to your config to initialize `hyprload`
    - `exec-once=$HOME/.local/share/hyprload/hyprload.sh`

# Usage
Put plugin builds you'd like to use into the `~/.local/share/hyprload/plugins/bin` directory. `hyprload` will load them automatically.

Hyprload keeps "sessions" of plugins, meaning that changing the plugins in `bin` (for example, updating them) does not require a full restart of Hyprload. The original unload will be called, the old plugin will be discarded, the new plugin copy will be loaded.
