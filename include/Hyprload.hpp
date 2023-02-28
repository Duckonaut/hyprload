#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <src/helpers/Color.hpp>

namespace hyprload {
    const CColor s_pluginColor = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};
    const CColor s_debugColor = {0x98 / 255.0f, 0x5f / 255.0f, 0xdd / 255.0f, 1.0f};

    const std::string c_pluginRoot = "plugin:hyprload:root";
    const std::string c_pluginQuiet = "plugin:hyprload:quiet";
    const std::string c_pluginDebug = "plugin:hyprload:debug";

    std::filesystem::path getRootPath();
    std::filesystem::path getPluginsPath();
    std::filesystem::path getPluginBinariesPath();

    bool isQuiet();
    bool isDebug();

    void log(const std::string& message);
    void debug(const std::string& message);

    class Hyprload {
      public:
        Hyprload();
        ~Hyprload();

        void dispatch(const std::string& command);
        void loadPlugins();
        void clearPlugins();
        void reloadPlugins();

        const std::vector<std::string>& getLoadedPlugins();
        bool isPluginLoaded(const std::string& name);

      private:
        std::optional<std::filesystem::path> getSessionBinariesPath();

        std::string generateSessionGuid();

        std::vector<std::string> m_vPlugins;
        std::optional<std::string> m_sSessionGuid;
    };

    inline std::unique_ptr<Hyprload> g_pHyprload;
}
