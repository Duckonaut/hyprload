#define WLR_USE_UNSTABLE
#include "src/helpers/Color.hpp"
#include "src/managers/KeybindManager.hpp"

#include "globals.hpp"

#include <src/Compositor.hpp>
#include <src/config/ConfigManager.hpp>
#include <src/helpers/Workspace.hpp>

#include <algorithm>
#include <filesystem>
#include <thread>
#include <unistd.h>
#include <vector>

const std::string s_pluginRoot = "plugin:hyprload:root";
const CColor s_pluginColor = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};

std::vector<std::string> g_vLoadedPlugins = std::vector<std::string>();

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
}

std::string getRootPath()
{
    static SConfigValue* hyprloadRoot = HyprlandAPI::getConfigValue(PHANDLE, s_pluginRoot);

    return hyprloadRoot->strValue;
}

std::string getPluginPath()
{
    return getRootPath() + "/plugins";
}

std::string getPluginBinaryPath()
{
    return getPluginPath() + "/bin";
}

void clearPlugins()
{
    for (const auto& plugin : g_vLoadedPlugins) {
        if (plugin.find(".so") != std::string::npos) {
            HyprlandAPI::addNotification(PHANDLE, "[hyprload] Unloading plugin: " + plugin, s_pluginColor, 5000);

            std::string pluginPath = getPluginBinaryPath() + "/" + plugin;

            HyprlandAPI::invokeHyprctlCommand("plugin", "unload " + pluginPath);
        }
    }
}

void loadPlugins()
{
    clearPlugins();

    std::vector<std::string> pluginFiles = std::vector<std::string>();

    for (const auto& entry : std::filesystem::directory_iterator(getPluginBinaryPath())) {
        pluginFiles.push_back(entry.path().filename());
    }

    for (auto& plugin : pluginFiles) {
        if (plugin.find(".so") != std::string::npos) {
            HyprlandAPI::addNotification(PHANDLE, "[hyprload] Loading plugin: " + plugin, s_pluginColor, 5000);

            std::string pluginPath = getPluginBinaryPath() + "/" + plugin;

            HyprlandAPI::invokeHyprctlCommand("plugin", "load " + pluginPath);

            g_vLoadedPlugins.push_back(plugin);
        }
    }
}

void reloadPlugins()
{
    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Reloading plugins...", s_pluginColor, 5000);

    clearPlugins();

    loadPlugins();

    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Plugins reloaded!", s_pluginColor, 5000);
}

void hyprloadDispatcher(std::string command)
{
    if (command == "reload") {
        reloadPlugins();
    }
    else if (command == "load") {
        loadPlugins();
    }
    else if (command == "clear") {
        clearPlugins();
    }
    else {
        HyprlandAPI::addNotification(PHANDLE, "[hyprload] Unknown command: " + command, s_pluginColor, 5000);
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    std::string defaultPluginDir = getenv("HOME") + std::string("/.local/share/hyprload/");

    HyprlandAPI::addConfigValue(PHANDLE, s_pluginRoot, SConfigValue{.strValue = defaultPluginDir});

    HyprlandAPI::addDispatcher(PHANDLE, "hyprload", hyprloadDispatcher);

    HyprlandAPI::reloadConfig();

    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Initialized successfully!", s_pluginColor, 5000);

    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Loading plugins...", s_pluginColor, 5000);

    loadPlugins();

    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Plugins loaded!", s_pluginColor, 5000);

    return {"hyprload", "Minimal hyprland plugin manager", "Duckonaut", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT()
{
    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Unloading plugins...", s_pluginColor, 5000);

    clearPlugins();

    HyprlandAPI::addNotification(PHANDLE, "[hyprload] Unloaded successfully!", s_pluginColor, 5000);

    HyprlandAPI::removeDispatcher(PHANDLE, "hyprload");
}
