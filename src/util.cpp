#include "types.hpp"
#include "globals.hpp"
#include "util.hpp"

#include <filesystem>
#include <optional>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <src/config/ConfigManager.hpp>

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

    std::filesystem::path getHyprlandHeadersPath() {
        std::optional<std::filesystem::path> path = getConfigHyprlandHeadersPath();
        if (path.has_value()) {
            return path.value();
        }

        return getRootPath() / "hyprland";
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

    void log(const std::string& message, usize duration) {
        std::string logMessage = "[hyprload] " + message;
        if (!isQuiet()) {
            HyprlandAPI::addNotification(PHANDLE, logMessage, s_pluginColor, duration);
        }
        Debug::log(LOG, (' ' + logMessage).c_str());
    }

    void debug(const std::string& message, usize duration) {
        std::string debugMessage = "[hyprload] " + message;
        if (isDebug()) {
            HyprlandAPI::addNotification(PHANDLE, debugMessage, s_debugColor, duration);
        }
        Debug::log(LOG, (' ' + debugMessage).c_str());
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

        return std::make_tuple(exit, result);
    }
}
