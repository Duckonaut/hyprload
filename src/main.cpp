#include "globals.hpp"

#include <src/Compositor.hpp>
#include <src/render/Renderer.hpp>
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

#include "Hyprload.hpp"

inline CFunctionHook* g_pRenderAllClientsForMonitorHook = nullptr;
typedef void (*origRenderAllClientsForMonitor)(void*, const int&, timespec*);

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void hkRenderAllClientsForMonitor(void* thisptr, const int& monitor, timespec* time) {
    (*(origRenderAllClientsForMonitor)g_pRenderAllClientsForMonitorHook->m_pOriginal)(thisptr, monitor, time);

    hyprload::g_pOverlay->drawOverlay(monitor, time);
}

void hyprloadDispatcher(std::string command) {
    hyprload::g_pHyprload->dispatch(command);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;
    hyprload::g_pHyprload = std::make_unique<hyprload::Hyprload>();
    hyprload::g_pOverlay = std::make_unique<hyprload::HyprloadOverlay>();

    std::string defaultPluginDir = getenv("HOME") + std::string("/.local/share/hyprload/");

    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginRoot, SConfigValue{.strValue = defaultPluginDir});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginQuiet, SConfigValue{.intValue = 0});
    HyprlandAPI::addConfigValue(PHANDLE, hyprload::c_pluginDebug, SConfigValue{.intValue = 0});

    HyprlandAPI::addDispatcher(PHANDLE, "hyprload", hyprloadDispatcher);

    g_pRenderAllClientsForMonitorHook =
        HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CHyprRenderer::renderAllClientsForMonitor, (void*)&hkRenderAllClientsForMonitor);

    g_pRenderAllClientsForMonitorHook->hook();

    HyprlandAPI::reloadConfig();

    hyprload::log("Initialized successfully!");

    hyprload::log("Loading plugins...");

    hyprload::g_pHyprload->loadPlugins();

    hyprload::log("Plugins loaded!");

    return {"hyprload", "Minimal hyprland plugin manager", "Duckonaut", "0.2.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprload::debug("Unloading plugin...");

    hyprload::g_pHyprload->cleanupPlugin();

    hyprload::debug("Unloaded successfully!");
}
