#include "globals.hpp"

#include "Hyprload.hpp"
#include <src/config/ConfigManager.hpp>
#include <src/plugins/PluginAPI.hpp>

namespace hyprload {
    std::filesystem::path getRootPath() {
        static SConfigValue* hyprloadRoot = HyprlandAPI::getConfigValue(PHANDLE, c_pluginRoot);

        return std::filesystem::path(hyprloadRoot->strValue);
    }

    std::filesystem::path getPluginsPath() {
        return getRootPath() / "plugins";
    }

    std::filesystem::path getPluginBinariesPath() {
        return getPluginsPath() / "bin";
    }

    bool isQuiet() {
        static SConfigValue* hyprloadQuiet = HyprlandAPI::getConfigValue(PHANDLE, c_pluginQuiet);

        return hyprloadQuiet->intValue;
        ;
    }

    bool isDebug() {
        static SConfigValue* hyprloadDebug = HyprlandAPI::getConfigValue(PHANDLE, c_pluginDebug);

        return hyprloadDebug->intValue;
    }

    void log(const std::string& message) {
        if (!isQuiet()) {
            HyprlandAPI::addNotification(PHANDLE, "[hyprload] " + message, s_pluginColor, 5000);
        }
    }

    void debug(const std::string& message) {
        if (isDebug()) {
            HyprlandAPI::addNotification(PHANDLE, "[hyprload] " + message, s_debugColor, 5000);
        }
    }

    Hyprload::Hyprload() {
        m_sSessionGuid = std::nullopt;
        m_vPlugins = std::vector<std::string>();
    }

    Hyprload::~Hyprload() {
        clearPlugins();
    }

    void Hyprload::dispatch(const std::string& command) {
        if (command == "load") {
            loadPlugins();
        } else if (command == "clear") {
            clearPlugins();
        } else if (command == "reload") {
            reloadPlugins();
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

        std::filesystem::path sourcePluginPath = getPluginBinariesPath();
        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();

        std::filesystem::create_directories(sessionPluginPath);

        std::vector<std::string> pluginFiles = std::vector<std::string>();

        for (const auto& entry : std::filesystem::directory_iterator(sourcePluginPath)) {
            std::string filename = entry.path().filename();
            pluginFiles.push_back(filename);
            std::filesystem::copy(entry.path(), sessionPluginPath / filename);
        }

        for (auto& plugin : pluginFiles) {
            if (plugin.find(".so") != std::string::npos) {
                log("Loading plugin: " + plugin);

                std::string pluginPath = sessionPluginPath / plugin;

                HyprlandAPI::invokeHyprctlCommand("plugin", "load " + pluginPath);

                m_vPlugins.push_back(plugin);
            }
        }
    }

    void Hyprload::clearPlugins() {
        if (!m_sSessionGuid.has_value()) {
            debug("Session guid does not exist, will not clear plugins...");
            return;
        }

        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();

        for (auto& plugin : m_vPlugins) {
            log("Unloading plugin: " + plugin);

            std::string pluginPath = sessionPluginPath / plugin;

            HyprlandAPI::invokeHyprctlCommand("plugin", "unload " + pluginPath);
        }

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

    std::string Hyprload::generateSessionGuid() {
        std::string guid = std::string();

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
