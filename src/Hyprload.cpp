#include "HyprloadOverlay.hpp"
#include "globals.hpp"

#include "Hyprload.hpp"
#include "src/helpers/Monitor.hpp"
#include "src/plugins/PluginSystem.hpp"
#include <cstdlib>
#include <ctime>
#include <src/config/ConfigManager.hpp>
#include <src/plugins/PluginAPI.hpp>

#include <time.h>
#include <random>

namespace hyprload {
    std::filesystem::path getRootPath() {
        SConfigValue* hyprloadRoot = HyprlandAPI::getConfigValue(PHANDLE, c_pluginRoot);

        return std::filesystem::path(hyprloadRoot->strValue);
    }

    std::filesystem::path getPluginsPath() {
        return getRootPath() / "plugins";
    }

    std::filesystem::path getPluginBinariesPath() {
        return getPluginsPath() / "bin";
    }

    bool isQuiet() {
        SConfigValue* hyprloadQuiet = HyprlandAPI::getConfigValue(PHANDLE, c_pluginQuiet);

        return hyprloadQuiet->intValue;
    }

    bool isDebug() {
        SConfigValue* hyprloadDebug = HyprlandAPI::getConfigValue(PHANDLE, c_pluginDebug);

        return hyprloadDebug->intValue;
    }

    void log(const std::string& message) {
        if (!isQuiet()) {
            HyprlandAPI::addNotification(PHANDLE, "[hyprload] " + message, s_pluginColor, 5000);
        }
    }

    void debug(const std::string& message) {
        if (isDebug()) {
            std::string debugMessage = "[hyprload] " + message;
            HyprlandAPI::addNotification(PHANDLE, debugMessage, s_debugColor, 5000);
            Debug::log(LOG, (' ' + debugMessage).c_str());
        }
    }

    Hyprload::Hyprload() {
        m_sSessionGuid = std::nullopt;
        m_vPlugins = std::vector<std::string>();
    }

    void Hyprload::dispatch(const std::string& command) {
        if (command == "load") {
            loadPlugins();
        } else if (command == "clear") {
            clearPlugins();
        } else if (command == "reload") {
            reloadPlugins();
        } else if (command == "overlay") {
            log("Toggling overlay...");
            g_pOverlay->m_bDrawOverlay = !g_pOverlay->m_bDrawOverlay;
        } else {
            log("Unknown command: " + command);
        }
    }

    void Hyprload::loadPlugins() {
        if (m_sSessionGuid.has_value()) {
            debug("Session guid already exists, will not load plugins...");
            return;
        }

        m_sSessionGuid = generateSessionGuid();

        debug("Session guid: " + m_sSessionGuid.value());

        std::filesystem::path sourcePluginPath = getPluginBinariesPath();
        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();

        debug("Creating session plugin path: " + sessionPluginPath.string());

        std::filesystem::create_directories(sessionPluginPath);

        debug("Copying plugins...");

        std::vector<std::string> pluginFiles = std::vector<std::string>();

        for (const auto& entry : std::filesystem::directory_iterator(sourcePluginPath)) {
            std::string filename = entry.path().filename();
            if (filename.find(".so") != std::string::npos) {
                debug("Discovered plugin: " + filename);

                pluginFiles.push_back(filename);

                debug("Copying plugin: " + entry.path().string() + " to " + (sessionPluginPath / filename).string());
                std::filesystem::copy(entry.path(), sessionPluginPath / filename);
            }
        }

        for (auto& plugin : pluginFiles) {
            log("Loading plugin: " + plugin);

            std::string pluginPath = sessionPluginPath / plugin;

            HyprlandAPI::invokeHyprctlCommand("plugin", "load " + pluginPath);

            m_vPlugins.push_back(plugin);
        }
    }

    void Hyprload::clearPlugins() {
        if (!m_sSessionGuid.has_value()) {
            debug("Session guid does not exist, will not clear plugins...");
            return;
        }

        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();
        std::vector<CPlugin*> plugins = g_pPluginSystem->getAllPlugins();

        for (auto& plugin : m_vPlugins) {
            log("Unloading plugin: " + plugin);

            std::string pluginPath = sessionPluginPath / plugin;

            if (std::none_of(plugins.begin(), plugins.end(), [&pluginPath](CPlugin* plugin) { return plugin->path == pluginPath; })) {
                debug("Plugin not found in plugin system, likely already unloaded, skipping unload...");
                continue;
            }

            HyprlandAPI::invokeHyprctlCommand("plugin", "unload " + pluginPath);
        }

        cleanupPlugin();
    }

    void Hyprload::cleanupPlugin() {
        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();

        m_vPlugins.clear();

        std::filesystem::remove_all(sessionPluginPath);

        m_sSessionGuid = std::nullopt;
    }

    void Hyprload::reloadPlugins() {
        log("Reloading plugins...");

        clearPlugins();
        loadPlugins();

        log("Reloaded plugins!");
    }

    const std::vector<std::string>& Hyprload::getLoadedPlugins() {
        return m_vPlugins;
    }

    std::string Hyprload::generateSessionGuid() {
        std::string guid = std::string();
        std::srand(std::time(nullptr));

        for (int i = 0; i < 16; i++) {
            guid += std::to_string(rand() % 10);
        }

        return guid;
    }

    std::optional<std::filesystem::path> Hyprload::getSessionBinariesPath() {
        if (!m_sSessionGuid.has_value()) {
            return std::nullopt;
        }

        return getPluginsPath() / ("session." + m_sSessionGuid.value());
    }

}
