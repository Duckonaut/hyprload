![Hyprload](./assets/hyprload_header.svg)

### A (WIP) [Hyprland](https://github.com/hyprwm/Hyprland) plugin manager

# Features
- [x] Loading plugins
- [x] Reloading plugins
- [x] Installing and updating plugins automatically
    - [x] A unified plugin manifest format
    - [x] Installing from git
    - [x] Installing from local
    - [ ] Installing from a url
    - [ ] Self-hosting

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

# Setup
1. To have hyprload manage your plugin installation, create a `hyprload.toml` file (by default, next to your `hyprland.conf` config: `~/.config/hypr/hyprload.toml`
2. Fill it out with the plugins you want. It consists of one array, named plugins, in which you can provide the installation source in various ways. Example:
```toml
plugins = [
    # Installs the plugin from https://github.com/Duckonaut/split-monitor-workspaces
    "Duckonaut/split-monitor-workspaces",
    # A more explicit definition of the git install
    { git = "https://github.com/Duckonaut/split-monitor-workspaces", branch = "main", name = "split-monitor-workspaces" },
    # Installs the same plugin from a local folder
    { local = "/home/duckonaut/repos/split-monitor-workspaces" },
]
```
3. Add keybinds to the `hyprload` dispatcher in your `hyprland.conf` for the functions you want.
    - Possible values:
        - `load`: Loads all the plugins
        - `clear`: Unloads all the plugins
        - `reload`: Unloads then reloads all the plugins
        - `overlay`: Toggles an overlay showing your actively loaded plugins
        - `install`: Installs the required plugins from `hyprload.toml`
        - `update`: Updates the required plugins from `hyprload.toml`
    - Example:
```
bind=SUPERSHIFT,R,hyprload,reload
bind=SUPERSHIFT,U,hyprload,update
bind=SUPERSHIFT,O,hyprload,overlay
```

## Configuration
The configuration of hyprload behavior is done in `hyprland.conf`, like a normal plugin
| Name                                      | Type      | Default                       | Description                                                   |
|-------------------------------------------|-----------|-------------------------------|---------------------------------------------------------------|
| `plugin:hyprload:quiet`                   | bool      | false                         | Whether to hide the notifications                             |
| `plugin:hyprload:debug`                   | bool      | false                         | Whether to hide extra-special debug notifications             |
| `plugin:hyprload:config`                  | string    | `~/.config/hypr/hyprload.toml`| The path to your plugin requirements file                     |
| `plugin:hyprload:hyprload_headers`        | string    | `empty`                       | The path to the Hyprland source to force using as headers.    |
