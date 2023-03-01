#include "globals.hpp"

#include <src/Compositor.hpp>
#include <src/render/Renderer.hpp>
#include <src/config/ConfigManager.hpp>
#include <src/helpers/Color.hpp>
#include <src/helpers/Workspace.hpp>
#include <src/managers/KeybindManager.hpp>
#include <src/managers/AnimationManager.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Hyprload.hpp"
#include "HyprloadOverlay.hpp"

inline CFunctionHook* g_pRenderAllClientsForMonitorHook = nullptr;
typedef void (*origRenderAllClientsForMonitor)(void*, const int&, timespec*);

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void hkRenderAllClientsForMonitor(void* thisptr, const int& monitor, timespec* time) {
    (*(origRenderAllClientsForMonitor)g_pRenderAllClientsForMonitorHook->m_pOriginal)(thisptr, monitor, time);

    hyprload::overlay::g_pOverlay->drawOverlay(monitor, time);
}

void hyprloadDispatcher(std::string command) {
    if (command == "load") {
        hyprload::g_pHyprload->loadPlugins();
    } else if (command == "clear") {
        hyprload::g_pHyprload->clearPlugins();
    } else if (command == "reload") {
        hyprload::g_pHyprload->reloadPlugins();
    } else if (command == "overlay") {
        hyprload::overlay::g_pOverlay->toggleDrawOverlay();
    } else {
        hyprload::log("Unknown command: " + command);
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;
    hyprload::g_pHyprload = std::make_unique<hyprload::Hyprload>();
    hyprload::overlay::g_pOverlay = std::make_unique<hyprload::overlay::HyprloadOverlay>();

    std::string home = getenv("HOME");
    std::string defaultPluginDir = home + std::string("/.local/share/hyprload/");
    std::string defaultConfigPath = home + std::string("/.config/hypr/hyprload.toml");

    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginRoot, SConfigValue{.strValue = defaultPluginDir});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginConfig, SConfigValue{.strValue = defaultConfigPath});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_hyprlandHeaders, SConfigValue{.strValue = ""});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginQuiet, SConfigValue{.intValue = 0});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginDebug, SConfigValue{.intValue = 0});

    HyprlandAPI::addConfigValue(PHANDLE, hyprload::overlay::c_overlayAnimationCurve, SConfigValue{.strValue = "default"});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::overlay::c_overlayAnimationDuration, SConfigValue{.floatValue = 0.5});

    HyprlandAPI::addDispatcher(PHANDLE, "hyprload", hyprloadDispatcher);

    g_pRenderAllClientsForMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CHyprRenderer::renderAllClientsForMonitor, (void*)&hkRenderAllClientsForMonitor);

    g_pRenderAllClientsForMonitorHook->hook();

    HyprlandAPI::reloadConfig();

    hyprload::log("Cleaning up old sessions...");

    hyprload::tryCleanupPreviousSessions();

    hyprload::log("Initialized successfully!");

    hyprload::log("Loading plugins...");

    hyprload::g_pHyprload->loadPlugins();

    hyprload::log("Plugins loaded!");

    return {"hyprload", "Minimal hyprland plugin manager", "Duckonaut", "0.3.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprload::debug("Unloading plugin...");

    hyprload::g_pHyprload->cleanupPlugin();

    hyprload::debug("Unloaded successfully!");
}
