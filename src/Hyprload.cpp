#include "HyprloadPlugin.hpp"
#include "globals.hpp"

#include "Hyprload.hpp"
#include "HyprloadConfig.hpp"
#include "HyprloadOverlay.hpp"

#include <mutex>
#include <src/helpers/Monitor.hpp>
#include <src/plugins/PluginSystem.hpp>
#include <src/config/ConfigManager.hpp>
#include <src/plugins/PluginAPI.hpp>

#include <thread>
#include <time.h>
#include <random>
#include <cstdlib>
#include <ctime>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <variant>
#include <vector>

namespace hyprload {
    std::filesystem::path getRootPath() {
        static SConfigValue* hyprloadRoot = HyprlandAPI::getConfigValue(PHANDLE, c_pluginRoot);

        return std::filesystem::path(hyprloadRoot->strValue);
    }

    std::optional<std::filesystem::path> getHyprlandHeadersPath() {
        static SConfigValue* hyprloadHeaders = HyprlandAPI::getConfigValue(PHANDLE, c_hyprlandHeaders);

        if (hyprloadHeaders->strValue.empty() || hyprloadHeaders->strValue == STRVAL_EMPTY) {
            return std::nullopt;
        }

        debug("Using Hyprland headers from: " + hyprloadHeaders->strValue);

        return std::filesystem::path(hyprloadHeaders->strValue);
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

    BuildProcessDescriptor::BuildProcessDescriptor(std::string&& name, std::shared_ptr<hyprload::plugin::PluginSource> source, const std::filesystem::path& hyprlandHeadersPath) {
        m_sName = std::move(name);
        m_pSource = source;
        m_sHyprlandHeadersPath = hyprlandHeadersPath;
        m_rResult = std::nullopt;
    }

    Hyprload::Hyprload() {
        m_sSessionGuid = std::nullopt;
        m_vPlugins = std::vector<std::string>();
        m_vBuildProcesses = std::vector<std::shared_ptr<hyprload::BuildProcessDescriptor>>();
    }

    void Hyprload::handleTick() {
        if (!m_bIsBuilding) {
            return;
        }

        auto buildProcesses = m_vBuildProcesses;
        for (auto bp : buildProcesses) {
            if (bp->m_mMutex.try_lock()) {
                if (bp->m_rResult.has_value()) {
                    if (bp->m_rResult.value().isErr()) {
                        log(bp->m_rResult.value().unwrapErr());
                    } else {
                        log("Successfully updated " + bp->m_sName);
                    }
                    m_vBuildProcesses.erase(std::remove(m_vBuildProcesses.begin(), m_vBuildProcesses.end(), bp), m_vBuildProcesses.end());
                    debug("Build processes left: " + std::to_string(m_vBuildProcesses.size()));
                }

                bp->m_mMutex.unlock();
            }
        }

        if (m_vBuildProcesses.empty()) {
            m_bIsBuilding = false;
            log("Finished updating all plugins");

            reloadPlugins();
        }
    }

    void Hyprload::installPlugins() {
        if (m_bIsBuilding) {
            log("Already updating plugins");
            return;
        }

        m_bIsBuilding = true;

        std::optional<std::filesystem::path> hyprlandHeadersPathMaybe = hyprload::getHyprlandHeadersPath();

        if (!hyprlandHeadersPathMaybe.has_value()) {
            hyprload::log("Could not find hyprland headers. Refer to https://github.com/Duckonaut/hyprload#Setup", 10000);
            return;
        }

        std::filesystem::path hyprlandHeadersPath = hyprlandHeadersPathMaybe.value();

        config::g_pHyprloadConfig->reloadConfig();

        const std::vector<plugin::PluginRequirement>& requirements = config::g_pHyprloadConfig->getPlugins();

        for (const plugin::PluginRequirement& plugin : requirements) {
            std::shared_ptr<hyprload::BuildProcessDescriptor> descriptor = std::make_shared<hyprload::BuildProcessDescriptor>(std::string(plugin.getName()), plugin.getSource(), hyprlandHeadersPath);

            auto myDescriptor = descriptor;
            m_vBuildProcesses.push_back(myDescriptor);

            std::thread thread = std::thread([descriptor]() {
                auto source = descriptor->m_pSource;

                if (!source->isSourceAvailable()) {
                    auto result = source->installSource();

                    if (result.isErr()) {
                        auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                        descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err("Failed to install " + descriptor->m_sName + " source: " + result.unwrapErr());
                        return;
                    }
                }

                auto result = source->build(descriptor->m_sName, descriptor->m_sHyprlandHeadersPath);

                if (result.isErr()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err("Failed to build " + descriptor->m_sName + ": " + result.unwrapErr());
                    return;
                }

                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::ok(std::monostate());
            });

            thread.detach();
        }
    }

    void Hyprload::updatePlugins() {
        if (m_bIsBuilding) {
            log("Already updating plugins");
            return;
        }

        m_bIsBuilding = true;

        std::optional<std::filesystem::path> hyprlandHeadersPathMaybe = hyprload::getHyprlandHeadersPath();

        if (!hyprlandHeadersPathMaybe.has_value()) {
            hyprload::log("Could not find hyprland headers. Refer to https://github.com/Duckonaut/hyprload#Setup", 10000);
            return;
        }

        std::filesystem::path hyprlandHeadersPath = hyprlandHeadersPathMaybe.value();

        config::g_pHyprloadConfig->reloadConfig();

        const std::vector<plugin::PluginRequirement>& requirements = config::g_pHyprloadConfig->getPlugins();

        for (const plugin::PluginRequirement& plugin : requirements) {
            std::shared_ptr<hyprload::BuildProcessDescriptor> descriptor = std::make_shared<hyprload::BuildProcessDescriptor>(std::string(plugin.getName()), plugin.getSource(), hyprlandHeadersPath);

            auto myDescriptor = descriptor;
            m_vBuildProcesses.push_back(myDescriptor);

            std::thread thread = std::thread([descriptor]() {
                auto source = descriptor->m_pSource;

                if (!source->isSourceAvailable()) {
                    auto result = source->installSource();

                    if (result.isErr()) {
                        auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                        descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err("Failed to install " + descriptor->m_sName + " source: " + result.unwrapErr());
                        return;
                    }
                }

                if (source->isUpToDate()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err("Source is up to date, skipping update...");
                    return;
                }

                auto result = source->update(descriptor->m_sName, descriptor->m_sHyprlandHeadersPath);

                if (result.isErr()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err("Failed to update " + descriptor->m_sName + ": " + result.unwrapErr());
                    return;
                }
                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::ok(std::monostate());
            });

            thread.detach();
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

        while (std::filesystem::exists(sessionPluginPath)) {
            debug("Session plugin path already exists, possible guid collision, regenerating guid...");

            m_sSessionGuid = generateSessionGuid();

            debug("Session guid: " + m_sSessionGuid.value());

            sessionPluginPath = getSessionBinariesPath().value();
        }

        debug("Creating session plugin path: " + sessionPluginPath.string());

        std::filesystem::create_directories(sessionPluginPath);

        debug("Creating lock file...");

        if (!lockSession()) {
            debug("Failed to create lock file, something is seriously wrong, will not load plugins...");

            return;
        }

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

        debug("Plugin count: " + std::to_string(plugins.size()));

        std::vector<std::string> pluginFiles = std::vector<std::string>();

        for (auto& plugin : m_vPlugins) {
            log("Unloading plugin: " + plugin);

            std::string pluginPath = sessionPluginPath / plugin;

            if (std::none_of(plugins.begin(), plugins.end(), [&pluginPath](CPlugin* plugin) { return plugin->path == pluginPath; })) {
                debug("Plugin not found in plugin system, likely already unloaded, skipping unload...");
                continue;
            }

            pluginFiles.push_back(plugin);
        }

        for (auto& plugin : pluginFiles) {
            HyprlandAPI::invokeHyprctlCommand("plugin", "unload " + plugin);
        }

        cleanupPlugin();
    }

    void Hyprload::cleanupPlugin() {
        std::filesystem::path sessionPluginPath = getSessionBinariesPath().value();
        std::filesystem::path pluginBinariesPath = getPluginBinariesPath();

        m_vPlugins.clear();

        debug("Removing lock file...");

        unlockSession();

        std::filesystem::remove_all(sessionPluginPath);

        const std::vector<plugin::PluginRequirement>& requirements = config::g_pHyprloadConfig->getPlugins();

        for (auto& entry : std::filesystem::directory_iterator(pluginBinariesPath)) {
            std::string filename = entry.path().filename();
            if (filename.find(".so") != std::string::npos) {
                std::string pluginName = filename.substr(0, filename.find(".so"));

                if (std::none_of(requirements.begin(), requirements.end(),
                                 [&pluginName](const plugin::PluginRequirement& requirement) { return requirement.getName() == pluginName; })) {
                    debug("Plugin " + pluginName + " not in requirements, removing...");

                    std::filesystem::remove(entry.path());
                }
            }
        }

        m_sSessionGuid = std::nullopt;
    }

    void Hyprload::reloadPlugins() {
        log("Reloading plugins...");

        clearPlugins();
        loadPlugins();

        log("Reloaded plugins!");
    }

    const std::vector<std::string>& Hyprload::getLoadedPlugins() const {
        return m_vPlugins;
    }

    std::string Hyprload::generateSessionGuid() {
        std::string guid = std::string();
        std::srand(std::time(nullptr));

        for (usize i = 0; i < 16; i++) {
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

    bool Hyprload::lockSession() {
        std::filesystem::path lockFile = getSessionBinariesPath().value() / "lock";

        m_iSessionLock = tryCreateLock(lockFile);

        if (!m_iSessionLock.has_value()) {
            debug("Lock file already exists, and is locked. Possible session guid collision, retry");

            return false;
        } else if (m_iSessionLock.value() == -1) {
            debug("Failed to create lock file, something is seriously wrong, will not load plugins...");

            return false;
        }

        return true;
    }

    void Hyprload::unlockSession() {
        std::filesystem::path lockFile = getSessionBinariesPath().value() / "lock";

        if (m_iSessionLock.has_value()) {
            releaseLock(m_iSessionLock.value());
        }

        if (std::filesystem::exists(lockFile)) {
            std::filesystem::remove(lockFile);
        }
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

    void tryCleanupPreviousSessions() {
        std::filesystem::path pluginsPath = getPluginsPath();

        for (const auto& entry : std::filesystem::directory_iterator(pluginsPath)) {
            std::string sessionPath = entry.path().filename();
            if (sessionPath.find("session.") != std::string::npos) {
                debug("Found previous session: " + sessionPath);

                std::filesystem::path lockFile = entry.path() / "lock";
                if (std::filesystem::exists(lockFile)) {
                    std::optional<int> lock = tryGetLock(lockFile);

                    if (!lock.has_value()) {
                        debug("Lock file exists, and is locked, skipping session: " + sessionPath);
                    } else if (lock.value() == -1) {
                        debug("Failed to get lock file, something is wrong...");

                        releaseLock(lock.value());
                        std::filesystem::remove_all(entry.path());
                    } else {
                        debug("Lock file exists, but is not locked (possible crash without unloading), removing session: " + sessionPath);

                        releaseLock(lock.value());
                        std::filesystem::remove_all(entry.path());
                    }
                } else {
                    debug("Lock file does not exist, removing session: " + sessionPath);
                    std::filesystem::remove_all(entry.path());
                }
            }
        }
    }
}
