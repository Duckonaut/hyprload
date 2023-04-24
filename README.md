![Hyprload](./assets/hyprload_header.svg)

### A [Hyprland](https://github.com/hyprwm/Hyprland) plugin manager

# Features
- [x] Loading plugins
- [x] Reloading plugins
- [x] Installing and updating plugins automatically
    - [x] A unified plugin manifest format
    - [x] Installing from git
    - [x] Installing from local
    - [X] Self-hosting
- [ ] Plugin browser

# Installing
1. Install `hyprload`
    - `curl -sSL https://raw.githubusercontent.com/Duckonaut/hyprload/main/install.sh | bash`
2. Add this to your config to initialize `hyprload`
    - `exec-once=$HOME/.local/share/hyprload/hyprload.sh`

> **Warning**: Right now, the main supported distribution is Arch. I have not tested on other distros personally.
> And sadly, as of right now, the `hyprland` package in the official Arch `community` repo does not work with hyprload (does not provide the version info required)
> AUR packages (especially `-git`) will work.

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
        - `install`: Installs the required plugins from `hyprload.toml`
        - `update`: Updates `hyprload` and the required plugins from `hyprload.toml`
    - Example:
```
bind=SUPERSHIFT,R,hyprload,reload
bind=SUPERSHIFT,U,hyprload,update
```

## Configuration
The configuration of hyprload behavior is done in `hyprland.conf`, like a normal plugin
| Name                                      | Type      | Default                       | Description                                                   |
|-------------------------------------------|-----------|-------------------------------|---------------------------------------------------------------|
| `plugin:hyprload:quiet`                   | bool      | false                         | Whether to hide the notifications                             |
| `plugin:hyprload:debug`                   | bool      | false                         | Whether to hide extra-special debug notifications             |
| `plugin:hyprload:config`                  | string    | `~/.config/hypr/hyprload.toml`| The path to your plugin requirements file                     |
| `plugin:hyprload:hyprload_headers`        | string    | `empty`                       | The path to the Hyprland source to force using as headers.    |

# Plugin Development
If you maintain a plugin for Hyprland, to support automatic management via `hyprload.toml`, you need to create a `hyprload.toml` manifest in the root of your
repository. `hyprload` cannot assume the way your plugins are built.

## Format
The plugin manifest needs to specify some information about the plugins in the repo. If you're familiar with TOML, it means specifying two dictionaries per plugin -
`PLUGIN_NAME` and `PLUGIN_NAME.build`. The `PLUGIN_NAME` dictionary has basic information about the plugin like the version, the description and the author. These
are not used **currently**, and can be omitted if you don't want to bother for now.

The `PLUGIN_NAME.build` dictionary is much more important, as it holds 2 important values: `output`, which is the path to the built plugin `.so` from the repo root,
and `steps`, which holds the commands to run to build that `.so`. **`hyprload` will define `HYPRLAND_HEADERS`** while building the plugin, and guarantees the version
of the headers matches the Hyprland version you're running.

It's important to note that the `hyprload.toml` plugin manifest can hold *multiple plugins*. This allows you to define a single manifest for a monorepo.

The full specification of a `PLUGIN_NAME` dict:
| Name              | Type      | Description                           |
|-------------------|-----------|---------------------------------------|
| description       | string    | Short description                     |
| version           | string    | Version                               |
| author            | string    | Author                                |
| authors           | list      | Can be defined instead of `author`    |
| build.output      | string    | The path of the `.so` output          |
| build.steps       | list      | List of commands to build the `.so`   |

## Examples
### Single plugin
[split-monitor-workspaces](https;//github.com/duckonaut/split-monitor-workspaces)
```toml
[split-monitor-workspaces]
description = "Split monitor workspaces"
version = "1.0.0"
author = "Duckonaut"

[split-monitor-workspaces.build]
output = "split-monitor-workspaces.so"
steps = [
    "make all",
]
```

### Multiple plugins
Here's what the manifest would look like for the [official plugins](https://github.com/hyprwm/hyprland-plugins).
```toml
[borders-plus-plus]
description ="A plugin to add more borders to windows."
version = "1.0"
author = "Vaxry"

[borders-plus-plus.build]
output = "borders-plus-plus/borders-plus-plus.so"
steps = [
    "make -C borders-plus-plus all",
]

[csgo-vulkan-fix]
description = "A plugin to fix incorrect mouse offsets on csgo in Vulkan"
version = "1.0"
author = "Vaxry"

[csgo-vulkan-fix.build]
output = "csgo-vulkan-fix/csgo-vulkan-fix.so"
steps = [
    "make -C csgo-vulkan-fix all",
]

[hyprbars]
description = "A plugin to add title bars to windows"
version = "1.0"
author = "Vaxry"

[hyprbars.build]
output = "hyprbars/hyprbars.so"
steps = [
    "make -C hyprbars all",
]
```
