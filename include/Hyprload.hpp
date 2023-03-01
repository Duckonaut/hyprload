#pragma once
#include "HyprloadOverlay.hpp"
#define WLR_USE_UNSTABLE

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <src/helpers/Color.hpp>
#include <src/helpers/Monitor.hpp>
#include <src/render/Texture.hpp>

namespace hyprload {
    const CColor s_pluginColor = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};
    const CColor s_debugColor = {0x98 / 255.0f, 0x5f / 255.0f, 0xdd / 255.0f, 1.0f};

    const std::string c_pluginRoot = "plugin:hyprload:root";
    const std::string c_pluginConfig = "plugin:hyprload:config";
    const std::string c_hyprlandHeaders = "plugin:hyprload:hyprland_headers";
    const std::string c_pluginQuiet = "plugin:hyprload:quiet";
    const std::string c_pluginDebug = "plugin:hyprload:debug";

    std::filesystem::path getRootPath();
    std::optional<std::filesystem::path> getHyprlandHeadersPath();
    std::filesystem::path getPluginsPath();
    std::filesystem::path getPluginBinariesPath();

    bool isQuiet();
    bool isDebug();

    void log(const std::string& message);
    void debug(const std::string& message);

    std::optional<int> tryCreateLock(const std::filesystem::path& path);
    std::optional<int> tryGetLock(const std::filesystem::path& path);
    void releaseLock(int lock);

    void tryCleanupPreviousSessions();

    class Hyprload {
      public:
        Hyprload();

        void dispatch(const std::string& command);

        void loadPlugins();
        void reloadPlugins();

        bool lockSession();
        void unlockSession();

        // Unload all plugins, except *this* plugin
        void clearPlugins();

        // Cleanup specific to *this* plugin
        void cleanupPlugin();

        const std::vector<std::string>& getLoadedPlugins();
        bool isPluginLoaded(const std::string& name);

      private:
        std::optional<std::filesystem::path> getSessionBinariesPath();

        std::string generateSessionGuid();

        std::vector<std::string> m_vPlugins;
        std::optional<std::string> m_sSessionGuid;
        std::optional<int> m_iSessionLock;
    };

    inline std::unique_ptr<Hyprload> g_pHyprload;
}
