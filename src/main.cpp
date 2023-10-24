#include "globals.hpp"
#include "util.hpp"

#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/helpers/Workspace.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/managers/AnimationManager.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Hyprload.hpp"
#include "HyprloadConfig.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void hyprloadDispatcher(std::string command) {
    if (command == "load") {
        hyprload::g_pHyprload->loadPlugins();
    } else if (command == "clear") {
        hyprload::g_pHyprload->clearPlugins();
    } else if (command == "reload") {
        hyprload::g_pHyprload->reloadPlugins();
    } else if (command == "install") {
        hyprload::g_pHyprload->installPlugins();
    } else if (command == "update") {
        hyprload::g_pHyprload->updatePlugins();
    } else {
        hyprload::error("Unknown command: " + command);
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;
    hyprload::g_pHyprload = std::make_unique<hyprload::Hyprload>();

    std::string home = getenv("HOME");
    std::string defaultPluginDir = home + std::string("/.local/share/hyprload/");
    std::string defaultConfigPath = home + std::string("/.config/hypr/hyprload.toml");

    bool configWasCreated = true;
    configWasCreated = configWasCreated &&
        HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginRoot,
                                    SConfigValue{.strValue = defaultPluginDir});
    configWasCreated = configWasCreated &&
        HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_hyprlandHeaders,
                                    SConfigValue{.strValue = STRVAL_EMPTY});
    configWasCreated = configWasCreated &&
        HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginQuiet, SConfigValue{.intValue = 0});
    configWasCreated = configWasCreated &&
        HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginDebug, SConfigValue{.intValue = 0});

    configWasCreated = configWasCreated &&
        HyprlandAPI::addConfigValue(PHANDLE, hyprload::config::c_pluginConfig,
                                    SConfigValue{.strValue = defaultConfigPath});

    configWasCreated =
        configWasCreated && HyprlandAPI::addDispatcher(PHANDLE, "hyprload", hyprloadDispatcher);

    if (!configWasCreated) {
        hyprload::error("Failed to create config values!");
    }

    HyprlandAPI::reloadConfig();

    if (!hyprload::g_pHyprload->checkIfHyprloadFullyCompatible()) {
        hyprload::error("Hyprland commit hash mismatch", 10000);
        hyprload::error("Hyprload was built with a different version of Hyprland", 10000);
        hyprload::error("Please update hyprload with the hyprload update dispatcher", 10000);
    }

    HyprlandAPI::registerCallbackDynamic(PHANDLE, "tick", [](void*, SCallbackInfo&, std::any) {
        hyprload::g_pHyprload->handleTick();
    });

    hyprload::config::g_pHyprloadConfig = std::make_unique<hyprload::config::HyprloadConfig>();

    hyprload::success("Initialized successfully!");

    hyprload::info("Cleaning up old sessions...");

    hyprload::tryCleanupPreviousSessions();

    for (auto& plugin : hyprload::config::g_pHyprloadConfig->getPlugins()) {
        hyprload::debug("Want to load plugin: " + plugin.getName() +
                        "\n\tBinary Path: " + plugin.getBinaryPath().string());
    }

    hyprload::info("Loading plugins...");

    hyprload::g_pHyprload->loadPlugins();

    hyprload::success("Plugins loaded!");

    return {"hyprload", "Hyprland plugin manager", "Duckonaut", "1.4.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprload::debug("Unloading plugin...");

    hyprload::g_pHyprload->cleanupPlugin();

    hyprload::debug("Unloaded successfully!");
}
