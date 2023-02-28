#include "globals.hpp"

#include <src/Compositor.hpp>
#include <src/config/ConfigManager.hpp>
#include <src/helpers/Color.hpp>
#include <src/helpers/Workspace.hpp>
#include <src/managers/KeybindManager.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <thread>
#include <unistd.h>
#include <vector>
#include <Hyprload.hpp>

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void hyprloadDispatcher(std::string command) {
    hyprload::g_pHyprload->dispatch(command);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;
    hyprload::g_pHyprload = std::make_unique<hyprload::Hyprload>();

    std::string defaultPluginDir = getenv("HOME") + std::string("/.local/share/hyprload/");

    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginRoot, SConfigValue{.strValue = defaultPluginDir});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginQuiet, SConfigValue{.intValue = 0});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginDebug, SConfigValue{.intValue = 1});

    HyprlandAPI::addDispatcher(PHANDLE, "hyprload", hyprloadDispatcher);

    HyprlandAPI::reloadConfig();

    hyprload::log("Initialized successfully!");

    hyprload::log("Loading plugins...");

    hyprload::g_pHyprload->loadPlugins();

    hyprload::log("Plugins loaded!");

    return {"hyprload", "Minimal hyprland plugin manager", "Duckonaut", "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprload::log("Unloading plugins...");

    hyprload::g_pHyprload->clearPlugins();

    hyprload::log("Unloaded successfully!");
}
