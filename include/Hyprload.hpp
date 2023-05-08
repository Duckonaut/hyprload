#pragma once
#define WLR_USE_UNSTABLE
#include "types.hpp"

#include "HyprloadPlugin.hpp"
#include "BuildProcessDescriptor.hpp"

#include <memory>
#include <mutex>
#include <variant>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <condition_variable>

#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/render/Texture.hpp>

namespace hyprload {
    void tryCleanupPreviousSessions();

    class Hyprload final {
      public:
        Hyprload();

        bool checkIfHyprloadFullyCompatible();

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
        const std::string& getCurrentHyprlandCommitHash();

      private:
        std::optional<std::filesystem::path> getSessionBinariesPath();
        std::string generateSessionGuid();
        void setupHeaders();
        std::string fetchHyprlandCommitHash();

        std::vector<std::string> m_vPlugins;
        std::optional<std::string> m_sSessionGuid;
        std::optional<flock_t> m_iSessionLock;

        std::string m_sHyprlandCommitNow;

        bool m_bIsBuilding = false;
        std::vector<std::shared_ptr<BuildProcessDescriptor>> m_vBuildProcesses;
    };

    inline std::unique_ptr<Hyprload> g_pHyprload;
}
