#!/bin/sh
# This script is used to install hyprload, in a known location

progress() {
    echo -e "\033[1;32m$1\033[0m"
}

HYPRLOAD_PATH=$HOME/.local/share/hyprload

HYPRLOAD_SOURCE_PATH="$HYPRLOAD_PATH/src"

progress "[1/7] Cloning hyprload to $HYPRLOAD_SOURCE_PATH"

if [ -d "$HYPRLOAD_SOURCE_PATH" ]; then
    git -C "$HYPRLOAD_SOURCE_PATH" pull
else
    git clone https://github.com/duckonaut/hyprload.git "$HYPRLOAD_SOURCE_PATH" --depth 1
fi

progress "[2/7] Cloned hyprload source to $HYPRLOAD_SOURCE_PATH"

HYPRLAND_PATH="$HYPRLOAD_PATH/include/hyprland"

progress "[3/7] Setting up hyprland source in $HYPRLAND_PATH"

if [ -d "$HYPRLAND_PATH" ]; then
    git -C "$HYPRLAND_PATH" pull
else
    git clone https://github.com/hyprwm/Hyprland.git "$HYPRLAND_PATH" --recursive
fi

HYPRLAND_COMMIT=""
if [ -z $(which hyprctl) ]; then
    HYPRLAND_COMMIT=$(git -C "$HYPRLAND_PATH" rev-parse HEAD)
else
    HYPRLAND_COMMIT=$(hyprctl version | grep "commit" | cut -d " " -f 8 | sed 's/dirty$//')
    git -C "$HYPRLAND_PATH" checkout "$HYPRLAND_COMMIT"
fi


progress "[4/7] Set up hyprland source in $HYPRLAND_PATH at commit $HYPRLAND_COMMIT"

progress "[5/7] Setting up hyprland plugin environment"

pkexec make -C "$HYPRLAND_PATH" pluginenv

export HYPRLAND_HEADERS="$HYPRLAND_PATH"

export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/share/pkgconfig"

progress "[6/7] Installing hyprload"

make -C "$HYPRLOAD_SOURCE_PATH" install

progress "[7/7] Installed hyprload!"
