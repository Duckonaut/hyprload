#!/bin/sh
# This script is used to load the hyprload plugin, accounting for an update

HYPRLOAD_PATH=$HOME/.local/share/hyprload

HYPRLOAD_SO="$HYPRLOAD_PATH/hyprload.so"
UPDATE_SO="$HYPRLOAD_PATH/hyprload.so.update"

if [ -f $UPDATE_SO ]; then
    mv $UPDATE_SO $HYPRLOAD_SO
fi

hyprctl plugin load $HYPRLOAD_SO
