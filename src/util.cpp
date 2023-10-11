#include "types.hpp"
#include "globals.hpp"
#include "util.hpp"

#include <filesystem>
#include <optional>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

namespace hyprload {
    std::filesystem::path getRootPath() {
        static SConfigValue* hyprloadRoot = HyprlandAPI::getConfigValue(PHANDLE, c_pluginRoot);

        return std::filesystem::path(hyprloadRoot->strValue);
    }

    std::optional<std::filesystem::path> getConfigHyprlandHeadersPath() {
        static SConfigValue* hyprloadHeaders =
            HyprlandAPI::getConfigValue(PHANDLE, c_hyprlandHeaders);

        if (hyprloadHeaders->strValue.empty() || hyprloadHeaders->strValue == STRVAL_EMPTY) {
            return std::nullopt;
        }

        debug("Using Hyprland headers from: " + hyprloadHeaders->strValue);

        return std::filesystem::path(hyprloadHeaders->strValue);
    }

    std::optional<std::filesystem::path> getHyprlandInstallationpath() {
        std::optional<std::filesystem::path> path = getConfigHyprlandHeadersPath();
        if (path.has_value()) {
            return std::nullopt;
        }

        return getRootPath() / "include" / "hyprland";
    }

    std::filesystem::path getHyprlandHeadersPath() {
        std::optional<std::filesystem::path> path = getConfigHyprlandHeadersPath();
        if (path.has_value()) {
            return path.value();
        }

        return getRootPath() / "include";
    }

    std::filesystem::path getPkgConfigOverridePath() {
        return getRootPath() / "pkgconfig";
    }

    std::filesystem::path getHyprlandPkgConfigPath() {
        return getPkgConfigOverridePath() / "hyprland.pc";
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
    }

    bool isDebug() {
        static SConfigValue* hyprloadDebug = HyprlandAPI::getConfigValue(PHANDLE, c_pluginDebug);

        return hyprloadDebug->intValue;
    }

    void info(const std::string& message, usize duration) {
        std::string logMessage = "[hyprload] " + message;
        if (!isQuiet()) {
            HyprlandAPI::addNotificationV2(PHANDLE,
                                           std::unordered_map<std::string, std::any>{
                                               {"text", logMessage},
                                               {"time", duration},
                                               {"color", CColor(0)},
                                               {"icon", eIcons::ICON_INFO},
                                           });
        }
        Debug::log(LOG, " {}", logMessage);
    }

    void success(const std::string& message, usize duration) {
        std::string logMessage = "[hyprload] " + message;
        if (!isQuiet()) {
            HyprlandAPI::addNotificationV2(PHANDLE,
                                           std::unordered_map<std::string, std::any>{
                                               {"text", logMessage},
                                               {"time", duration},
                                               {"color", CColor(0)},
                                               {"icon", eIcons::ICON_OK},
                                           });
        }
        Debug::log(LOG, " {}", logMessage);
    }

    void error(const std::string& message, usize duration) {
        std::string logMessage = "[hyprload] " + message;
        if (!isQuiet()) {
            HyprlandAPI::addNotificationV2(PHANDLE,
                                           std::unordered_map<std::string, std::any>{
                                               {"text", logMessage},
                                               {"time", duration},
                                               {"color", CColor(0)},
                                               {"icon", eIcons::ICON_ERROR},
                                           });
        }
        Debug::log(LOG, " {}", logMessage);
    }

    void debug(const std::string& message, usize duration) {
        std::string debugMessage = "[hyprload] " + message;
        if (isDebug()) {
            HyprlandAPI::addNotificationV2(PHANDLE,
                                           std::unordered_map<std::string, std::any>{
                                               {"text", debugMessage},
                                               {"time", duration},
                                               {"color", s_debugColor},
                                               {"icon", eIcons::ICON_INFO},
                                           });
        }
        Debug::log(LOG, " {}", debugMessage);
    }

    std::optional<int> tryCreateLock(const std::filesystem::path& lockFile) {
        mode_t oldMask = umask(0);
        fd_t fd = open(lockFile.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
        umask(oldMask);

        if (fd < 0) {
            close(fd);
            return -1;
        }

        flock_t lock = flock(fd, LOCK_EX | LOCK_NB);

        if (lock < 0) {
            close(fd);
            return std::nullopt;
        }

        return fd;
    }

    std::optional<int> tryGetLock(const std::filesystem::path& lockFile) {
        fd_t fd = open(lockFile.c_str(), O_RDWR);

        if (fd < 0) {
            close(fd);
            return std::nullopt;
        }

        flock_t lock = flock(fd, LOCK_EX | LOCK_NB);

        if (lock < 0) {
            close(fd);
            return std::nullopt;
        }

        return fd;
    }

    void releaseLock(flock_t lock) {
        if (lock < 0) {
            return;
        }

        flock(lock, LOCK_UN);
    }

    std::tuple<int, std::string> executeCommand(const std::string& command) {
        std::string result = "";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return std::make_tuple(-1, "Failed to execute command");
        }

        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr) {
                result += buffer;
            }
        }

        int exit = pclose(pipe);

        Debug::log(LOG, " [hyprload] Command: %s", command.c_str());
        Debug::log(LOG, " [hyprload] Exit code: %d", exit);
        Debug::log(LOG, " [hyprload] Result: %s", result.c_str());

        return std::make_tuple(exit, result);
    }
}
