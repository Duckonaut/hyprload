#include "HyprloadPlugin.hpp"
#include "globals.hpp"
#include "util.hpp"

#include "Hyprload.hpp"
#include "HyprloadConfig.hpp"
#include "HyprloadOverlay.hpp"

#include <src/helpers/Monitor.hpp>
#include <src/plugins/PluginSystem.hpp>
#include <src/config/ConfigManager.hpp>
#include <src/plugins/PluginAPI.hpp>

#include <thread>
#include <random>
#include <condition_variable>
#include <mutex>
#include <variant>
#include <vector>

namespace hyprload {
    std::mutex g_mSetupHeadersMutex;
    std::condition_variable g_cvSetupHeaders;
    std::optional<hyprload::Result<std::monostate, std::string>> g_bHeadersReady = std::nullopt;

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
                        error(bp->m_rResult.value().unwrapErr());
                    } else {
                        success("Successfully updated " + bp->m_sName);
                    }
                    m_vBuildProcesses.erase(
                        std::remove(m_vBuildProcesses.begin(), m_vBuildProcesses.end(), bp),
                        m_vBuildProcesses.end());
                    debug("Build processes left: " + std::to_string(m_vBuildProcesses.size()));
                }

                bp->m_mMutex.unlock();
            }
        }

        if (m_vBuildProcesses.empty()) {
            m_bIsBuilding = false;
            success("Finished updating all plugins");

            reloadPlugins();
        }
    }

    void Hyprload::installPlugins() {
        if (m_bIsBuilding) {
            error("Already updating plugins");
            return;
        }

        m_bIsBuilding = true;

        std::optional<std::filesystem::path> configHyprlandHeadersPath =
            hyprload::getConfigHyprlandHeadersPath();

        if (!configHyprlandHeadersPath.has_value()) {
            setupHeaders();
        } else {
            g_bHeadersReady = hyprload::Result<std::monostate, std::string>::ok(std::monostate());
        }

        std::filesystem::path hyprlandHeadersPath = getHyprlandHeadersPath();

        config::g_pHyprloadConfig->reloadConfig();

        const std::vector<plugin::PluginRequirement>& requirements =
            config::g_pHyprloadConfig->getPlugins();

        for (const plugin::PluginRequirement& plugin : requirements) {
            std::shared_ptr<hyprload::BuildProcessDescriptor> descriptor =
                std::make_shared<hyprload::BuildProcessDescriptor>(
                    std::string(plugin.getName()), plugin.getSource(), hyprlandHeadersPath);

            auto myDescriptor = descriptor;
            m_vBuildProcesses.push_back(myDescriptor);

            std::thread thread = std::thread([descriptor]() {
                std::unique_lock<std::mutex> headerLock = std::unique_lock(g_mSetupHeadersMutex);
                g_cvSetupHeaders.wait(headerLock, []() { return g_bHeadersReady.has_value(); });

                if (g_bHeadersReady.value().isOk()) {
                    headerLock.unlock();
                } else {
                    auto lock = std::unique_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to setup Hyprland headers");
                    return;
                }

                auto source = descriptor->m_pSource;

                if (!source->isSourceAvailable()) {
                    auto result = source->installSource();

                    if (result.isErr()) {
                        auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                        descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                            "Failed to install " + descriptor->m_sName +
                            " source: " + result.unwrapErr());
                        return;
                    }
                }

                auto result =
                    source->build(descriptor->m_sName, descriptor->m_sHyprlandHeadersPath);

                if (result.isErr()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to build " + descriptor->m_sName + ": " + result.unwrapErr());
                    return;
                }

                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult =
                    hyprload::Result<std::monostate, std::string>::ok(std::monostate());
            });

            thread.detach();
        }
    }

    void Hyprload::updatePlugins() {
        if (m_bIsBuilding) {
            error("Already updating plugins");
            return;
        }

        m_bIsBuilding = true;

        std::optional<std::filesystem::path> configHyprlandHeadersPath =
            hyprload::getConfigHyprlandHeadersPath();

        if (!configHyprlandHeadersPath.has_value()) {
            setupHeaders();
        } else {
            g_bHeadersReady = hyprload::Result<std::monostate, std::string>::ok(std::monostate());
        }

        std::filesystem::path hyprlandHeadersPath = getHyprlandHeadersPath();

        // update self

        std::shared_ptr<hyprload::BuildProcessDescriptor> descriptor =
            std::make_shared<hyprload::BuildProcessDescriptor>(
                "hyprload", std::make_shared<plugin::SelfSource>(), hyprlandHeadersPath);

        auto myDescriptor = descriptor;
        m_vBuildProcesses.push_back(myDescriptor);

        std::thread thread = std::thread([descriptor]() {
            std::unique_lock<std::mutex> headerLock = std::unique_lock(g_mSetupHeadersMutex);
            g_cvSetupHeaders.wait(headerLock, []() { return g_bHeadersReady.has_value(); });

            if (g_bHeadersReady.value().isOk()) {
                headerLock.unlock();
            } else {
                auto lock = std::unique_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                    "Failed to setup Hyprland headers");
                return;
            }

            auto source = descriptor->m_pSource;

            if (!source->isSourceAvailable()) {
                auto result = source->installSource();

                if (result.isErr()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to install " + descriptor->m_sName +
                        " source: " + result.unwrapErr());
                    return;
                }
            }

            if (source->isUpToDate()) {
                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                    "Source is up to date, skipping update...");
                return;
            }

            auto result = source->update(descriptor->m_sName, descriptor->m_sHyprlandHeadersPath);

            if (result.isErr()) {
                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                    "Failed to build " + descriptor->m_sName + ": " + result.unwrapErr());
                return;
            }

            auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

            descriptor->m_rResult =
                hyprload::Result<std::monostate, std::string>::ok(std::monostate());
        });

        thread.detach();

        config::g_pHyprloadConfig->reloadConfig();

        const std::vector<plugin::PluginRequirement>& requirements =
            config::g_pHyprloadConfig->getPlugins();

        for (const plugin::PluginRequirement& plugin : requirements) {
            std::shared_ptr<hyprload::BuildProcessDescriptor> descriptor =
                std::make_shared<hyprload::BuildProcessDescriptor>(
                    std::string(plugin.getName()), plugin.getSource(), hyprlandHeadersPath);

            auto myDescriptor = descriptor;
            m_vBuildProcesses.push_back(myDescriptor);

            std::thread thread = std::thread([descriptor]() {
                std::unique_lock<std::mutex> headerLock = std::unique_lock(g_mSetupHeadersMutex);
                g_cvSetupHeaders.wait(headerLock, []() { return g_bHeadersReady.has_value(); });

                if (g_bHeadersReady.value().isOk()) {
                    headerLock.unlock();
                } else {
                    auto lock = std::unique_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to setup Hyprland headers");
                    return;
                }

                auto source = descriptor->m_pSource;

                if (!source->isSourceAvailable()) {
                    auto result = source->installSource();

                    if (result.isErr()) {
                        auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                        descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                            "Failed to install " + descriptor->m_sName +
                            " source: " + result.unwrapErr());
                        return;
                    }
                }

                if (source->isUpToDate()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Source is up to date, skipping update...");
                    return;
                }

                auto result =
                    source->update(descriptor->m_sName, descriptor->m_sHyprlandHeadersPath);

                if (result.isErr()) {
                    auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                    descriptor->m_rResult = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to update " + descriptor->m_sName + ": " + result.unwrapErr());
                    return;
                }
                auto lock = std::scoped_lock<std::mutex>(descriptor->m_mMutex);

                descriptor->m_rResult =
                    hyprload::Result<std::monostate, std::string>::ok(std::monostate());
            });

            thread.detach();
        }
    }

    void Hyprload::setupHeaders() {
        std::string hyprlandVersion = HyprlandAPI::invokeHyprctlCommand("version", {}, "j");
        debug("Hyprland version: " + hyprlandVersion);

        usize hyprlandVersionStart = hyprlandVersion.find("\"commit\": \"");
        if (hyprlandVersionStart == std::string::npos) {
            error("Failed to find commit hash in Hyprland version");
            return;
        }
        std::string commitHash = hyprlandVersion.substr(hyprlandVersionStart + 11, 40);

        debug("Hyprland commit hash: " + commitHash);

        std::thread thread = std::thread([commitHash]() {
            std::unique_lock<std::mutex> headerLock = std::unique_lock(g_mSetupHeadersMutex);

            std::optional<std::filesystem::path> configHyprlandHeadersPath =
                hyprload::getConfigHyprlandHeadersPath();

            if (configHyprlandHeadersPath.has_value()) {
                g_bHeadersReady =
                    hyprload::Result<std::monostate, std::string>::ok(std::monostate());
                headerLock.unlock();
                g_cvSetupHeaders.notify_all();
                return;
            }

            std::filesystem::path hyprlandHeadersPath = hyprload::getHyprlandHeadersPath();

            if (!std::filesystem::exists(hyprlandHeadersPath)) {
                // Clone hyprland

                const std::string hyprlandUrl = "https://github.com/hyprwm/Hyprland.git";

                std::string command = "git clone " + hyprlandUrl + " " +
                    hyprlandHeadersPath.string() + " --recurse-submodules --depth 1";

                std::tuple<int, std::string> result = hyprload::executeCommand(command);

                if (std::get<0>(result) != 0) {
                    g_bHeadersReady = hyprload::Result<std::monostate, std::string>::err(
                        "Failed to clone Hyprland: " + std::get<1>(result));
                    headerLock.unlock();
                    g_cvSetupHeaders.notify_all();
                    return;
                }
            }

            // Checkout to commit hash
            std::string command = "git -C " + hyprlandHeadersPath.string() + " fetch && git -C " +
                hyprlandHeadersPath.string() + " checkout " + commitHash + " --recurse-submodules";

            std::tuple<int, std::string> result = hyprload::executeCommand(command);

            if (std::get<0>(result) != 0) {
                g_bHeadersReady = hyprload::Result<std::monostate, std::string>::err(
                    "Failed to checkout to commit hash: " + std::get<1>(result));
                headerLock.unlock();
                g_cvSetupHeaders.notify_all();
                return;
            }

            // Make pluginenv
            command = "make -C " + hyprlandHeadersPath.string() + " pluginenv";

            result = hyprload::executeCommand(command);

            if (std::get<0>(result) != 0) {
                g_bHeadersReady = hyprload::Result<std::monostate, std::string>::err(
                    "Failed to make pluginenv: " + std::get<1>(result));
                headerLock.unlock();
                g_cvSetupHeaders.notify_all();
                return;
            }

            debug("Hyprland headers ready");

            g_bHeadersReady = hyprload::Result<std::monostate, std::string>::ok(std::monostate());
            headerLock.unlock();
            g_cvSetupHeaders.notify_all();
        });

        thread.detach();
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
            debug("Session plugin path already exists, possible guid collision, regenerating "
                  "guid...");

            m_sSessionGuid = generateSessionGuid();

            debug("Session guid: " + m_sSessionGuid.value());

            sessionPluginPath = getSessionBinariesPath().value();
        }

        debug("Creating session plugin path: " + sessionPluginPath.string());

        std::filesystem::create_directories(sessionPluginPath);

        debug("Creating lock file...");

        if (!lockSession()) {
            debug("Failed to create lock file, something is seriously wrong, will not load "
                  "plugins...");

            return;
        }

        debug("Copying plugins...");

        std::vector<std::string> pluginFiles = std::vector<std::string>();

        for (const auto& entry : std::filesystem::directory_iterator(sourcePluginPath)) {
            std::string filename = entry.path().filename();
            if (filename.find(".so") != std::string::npos) {
                debug("Discovered plugin: " + filename);

                pluginFiles.push_back(filename);

                debug("Copying plugin: " + entry.path().string() + " to " +
                      (sessionPluginPath / filename).string());
                std::filesystem::copy(entry.path(), sessionPluginPath / filename);
            }
        }

        for (auto& plugin : pluginFiles) {
            info("Loading plugin: " + plugin);

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
            info("Unloading plugin: " + plugin);

            std::string pluginPath = sessionPluginPath / plugin;

            if (std::none_of(plugins.begin(), plugins.end(), [&pluginPath](CPlugin* plugin) {
                    return plugin->path == pluginPath;
                })) {
                debug("Plugin not found in plugin system, likely already unloaded, skipping "
                      "unload...");
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

        const std::vector<plugin::PluginRequirement>& requirements =
            config::g_pHyprloadConfig->getPlugins();

        for (auto& entry : std::filesystem::directory_iterator(pluginBinariesPath)) {
            std::string filename = entry.path().filename();
            if (filename.find(".so") != std::string::npos) {
                std::string pluginName = filename.substr(0, filename.find(".so"));

                if (std::none_of(requirements.begin(), requirements.end(),
                                 [&pluginName](const plugin::PluginRequirement& requirement) {
                                     return requirement.getName() == pluginName;
                                 })) {
                    debug("Plugin " + pluginName + " not in requirements, removing...");

                    std::filesystem::remove(entry.path());
                }
            }
        }

        m_sSessionGuid = std::nullopt;
    }

    void Hyprload::reloadPlugins() {
        info("Reloading plugins...");

        clearPlugins();
        loadPlugins();

        success("Reloaded plugins!");
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
            debug("Lock file already exists, and is locked. Possible session guid collision, "
                  "retry");

            return false;
        } else if (m_iSessionLock.value() == -1) {
            debug("Failed to create lock file, something is seriously wrong, will not load "
                  "plugins...");

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
                        debug("Lock file exists, but is not locked (possible crash without "
                              "unloading), removing session: " +
                              sessionPath);

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
