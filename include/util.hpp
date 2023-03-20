#pragma once
#include "types.hpp"

#include <filesystem>
#include <optional>

#include <src/helpers/Color.hpp>

namespace hyprload {
    const CColor s_pluginColor = {0x98 / 255.0f, 0xc3 / 255.0f, 0x79 / 255.0f, 1.0f};
    const CColor s_debugColor = {0x98 / 255.0f, 0x5f / 255.0f, 0xdd / 255.0f, 1.0f};

    const std::string c_pluginRoot = "plugin:hyprload:root";
    const std::string c_hyprlandHeaders = "plugin:hyprload:hyprland_headers";
    const std::string c_pluginQuiet = "plugin:hyprload:quiet";
    const std::string c_pluginDebug = "plugin:hyprload:debug";

    std::filesystem::path getRootPath();
    std::optional<std::filesystem::path> getConfigHyprlandHeadersPath();
    std::filesystem::path getHyprlandHeadersPath();
    std::filesystem::path getPluginsPath();
    std::filesystem::path getPluginBinariesPath();

    bool isQuiet();
    bool isDebug();

    void info(const std::string& message, usize duration = 5000);
    void success(const std::string& message, usize duration = 5000);
    void error(const std::string& message, usize duration = 5000);
    void debug(const std::string& message, usize duration = 5000);

    std::optional<flock_t> tryCreateLock(const std::filesystem::path& path);
    std::optional<flock_t> tryGetLock(const std::filesystem::path& path);
    void releaseLock(flock_t lock);

    std::tuple<int, std::string> executeCommand(const std::string& command);
}
