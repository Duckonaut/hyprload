#pragma once
#include "HyprloadPlugin.hpp"
#include <memory>
#include <mutex>
#include <variant>
#define WLR_USE_UNSTABLE
#include "HyprloadOverlay.hpp"
#include "types.hpp"

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
    const std::string c_hyprlandHeaders = "plugin:hyprload:hyprland_headers";
    const std::string c_pluginQuiet = "plugin:hyprload:quiet";
    const std::string c_pluginDebug = "plugin:hyprload:debug";

    std::filesystem::path getRootPath();
    std::optional<std::filesystem::path> getHyprlandHeadersPath();
    std::filesystem::path getPluginsPath();
    std::filesystem::path getPluginBinariesPath();

    bool isQuiet();
    bool isDebug();

    void log(const std::string& message, usize duration = 5000);
    void debug(const std::string& message, usize duration = 5000);

    std::optional<flock_t> tryCreateLock(const std::filesystem::path& path);
    std::optional<flock_t> tryGetLock(const std::filesystem::path& path);
    void releaseLock(flock_t lock);

    void tryCleanupPreviousSessions();

    class BuildProcessDescriptor final {
      public:
        BuildProcessDescriptor(std::string&& name,
                               std::shared_ptr<hyprload::plugin::PluginSource> source,
                               const std::filesystem::path& hyprlandHeadersPath);

        std::string m_sName;
        std::shared_ptr<hyprload::plugin::PluginSource> m_pSource;
        std::filesystem::path m_sHyprlandHeadersPath;

        std::mutex m_mMutex;
        std::optional<hyprload::Result<std::monostate, std::string>> m_rResult;
    };

    class Hyprload final {
      public:
        Hyprload();

        void handleTick();

        void installPlugins();
        void updatePlugins();

        void loadPlugins();
        void reloadPlugins();

        bool lockSession();
        void unlockSession();

        // Unload all plugins, except *this* plugin
        void clearPlugins();

        // Cleanup specific to *this* plugin
        void cleanupPlugin();

        const std::vector<std::string>& getLoadedPlugins() const;
        bool isPluginLoaded(const std::string& name) const;

      private:
        std::optional<std::filesystem::path> getSessionBinariesPath();

        std::string generateSessionGuid();

        std::vector<std::string> m_vPlugins;
        std::optional<std::string> m_sSessionGuid;
        std::optional<flock_t> m_iSessionLock;

        bool m_bIsBuilding = false;
        std::vector<std::shared_ptr<BuildProcessDescriptor>> m_vBuildProcesses;
    };

    inline std::unique_ptr<Hyprload> g_pHyprload;
}
