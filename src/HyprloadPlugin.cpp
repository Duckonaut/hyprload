#include "types.hpp"
#include "util.hpp"

#include "HyprloadPlugin.hpp"
#include "Hyprload.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <hyprland/src/helpers/MiscFunctions.hpp>
#include "toml/toml.hpp"

namespace hyprload::plugin {
    hyprload::Result<HyprloadManifest, std::string>
    getHyprloadManifest(const std::filesystem::path& sourcePath) {
        std::filesystem::path manifestPath = sourcePath / "hyprload.toml";

        if (!std::filesystem::exists(manifestPath)) {
            return hyprload::Result<HyprloadManifest, std::string>::err(
                "Source does not have a hyprload.toml manifest");
        }

        toml::table manifest;
        try {
            manifest = toml::parse_file(manifestPath.string());
        } catch (const std::exception& e) {
            return hyprload::Result<HyprloadManifest, std::string>::err(
                "Failed to parse source manifest: " + std::string(e.what()));
        }

        return hyprload::Result<HyprloadManifest, std::string>::ok(HyprloadManifest(manifest));
    }

    hyprload::Result<PluginManifest, std::string>
    getPluginManifest(const std::filesystem::path& sourcePath, const std::string& name) {

        auto hyprloadManifestResult = getHyprloadManifest(sourcePath);

        if (hyprloadManifestResult.isErr()) {
            return hyprload::Result<PluginManifest, std::string>::err(
                hyprloadManifestResult.unwrapErr());
        }

        const auto& hyprloadManifest = hyprloadManifestResult.unwrap();

        std::optional<PluginManifest> pluginManifest;

        for (const auto& plugin : hyprloadManifest.getPlugins()) {
            if (plugin.getName() == name) {
                pluginManifest = plugin;
                break;
            }
        }

        if (!pluginManifest.has_value()) {
            return hyprload::Result<PluginManifest, std::string>::err(
                "Plugin does not have a manifest for " + name);
        }

        return hyprload::Result<PluginManifest, std::string>::ok(std::move(pluginManifest.value()));
    }

    hyprload::Result<std::monostate, std::string>
    buildPlugin(const std::filesystem::path& sourcePath, const std::string& name) {
        auto pluginManifestResult = getPluginManifest(sourcePath, name);

        if (pluginManifestResult.isErr()) {
            return hyprload::Result<std::monostate, std::string>::err(
                pluginManifestResult.unwrapErr());
        }

        const auto& pluginManifest = pluginManifestResult.unwrap();

        std::string buildSteps = "export PKG_CONFIG_PATH=" + getPkgConfigOverridePath().string() +
            " && cd " + sourcePath.string() + " && ";

        for (const std::string& step : pluginManifest.getBuildSteps()) {
            buildSteps += step + " && ";
        }

        buildSteps += "cd -";

        auto [exit, output] = executeCommand(buildSteps);

        if (exit != 0) {
            return hyprload::Result<std::monostate, std::string>::err("Failed to build plugin: " +
                                                                      output);
        }

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    PluginManifest::PluginManifest(std::string&& name, const toml::table& manifest) {
        m_sName = name;

        std::vector<std::string> authors = std::vector<std::string>();

        if (manifest.contains("authors") && manifest["authors"].is_array()) {
            manifest["authors"].as_array()->for_each([&authors](const auto& value) {
                if (!value.is_string()) {
                    throw std::runtime_error("Author must be a string");
                }
                authors.push_back(value.as_string()->get());
            });
        } else if (manifest.contains("author")) {
            authors.push_back(manifest["author"].as_string()->get());
        }

        m_sAuthors = authors;

        if (manifest.contains("version")) {
            m_sVersion = manifest["version"].as_string()->get();
        } else {
            m_sVersion = "0.0.0";
        }

        if (manifest.contains("description")) {
            m_sDescription = manifest["description"].as_string()->get();
        } else {
            m_sDescription = "No description provided";
        }

        if (manifest.contains("build") && manifest["build"].is_table()) {
            const toml::table* build = manifest["build"].as_table();

            if (build->contains("output") && build->get("output")->is_string()) {
                m_pBinaryOutputPath = build->get("output")->as_string()->get();
            } else {
                m_pBinaryOutputPath = name + ".so";
            }

            if (build->contains("steps") && build->get("steps")->is_array()) {
                build->get("steps")->as_array()->for_each(
                    [&buildSteps = m_sBuildSteps](const toml::node& value) {
                        if (!value.is_string()) {
                            throw std::runtime_error("Build step must be a string");
                        }
                        buildSteps.push_back(value.as_string()->get());
                    });
            } else {
                throw std::runtime_error("Plugin must have build steps");
            }
        } else {
            throw std::runtime_error("Plugin must have a build table");
        }
    }

    const std::string& PluginManifest::getName() const {
        return m_sName;
    }

    const std::vector<std::string>& PluginManifest::getAuthors() const {
        return m_sAuthors;
    }

    const std::string& PluginManifest::getVersion() const {
        return m_sVersion;
    }

    const std::string& PluginManifest::getDescription() const {
        return m_sDescription;
    }

    const std::filesystem::path& PluginManifest::getBinaryOutputPath() const {
        return m_pBinaryOutputPath;
    }

    const std::vector<std::string>& PluginManifest::getBuildSteps() const {
        return m_sBuildSteps;
    }

    HyprloadManifest::HyprloadManifest(const toml::table& manifest) {
        m_vPlugins = std::vector<PluginManifest>();
        manifest.for_each([&plugins = m_vPlugins](const toml::key& key, const toml::node& value) {
            if (value.is_table()) {
                debug("Found plugin " + std::string(key.str()) + " in hyprload manifest");
                toml::table table = *value.as_table();
                plugins.emplace_back(std::string(key.str()), table);
            }
        });
    }

    const std::vector<PluginManifest>& HyprloadManifest::getPlugins() const {
        return m_vPlugins;
    }

    bool PluginSource::operator==(const PluginSource& other) const {
        if (typeid(*this) != typeid(other)) {
            return false;
        }

        return this->isEquivalent(other);
    }

    GitPluginSource::GitPluginSource(std::string&& url, std::optional<std::string>&& branch,
                                     std::optional<std::string>&& rev) {
        m_sBranch = branch;
        m_sRev = rev;

        if (url.find("https://") == 0) {
            m_sUrl = url;
        } else if (url.find("git@") == 0) {
            m_sUrl = url;
        } else {
            m_sUrl = "https://github.com/" + url + ".git";
        }

        std::string name = m_sUrl.substr(m_sUrl.find_last_of('/') + 1);
        name = name.substr(0, name.find_last_of('.'));

        m_pSourcePath = hyprload::getPluginsPath() / "src" / name;

        if (m_sBranch.has_value()) {
            m_pSourcePath += "@" + m_sBranch.value();
        }

        if (m_sRev.has_value()) {
            m_pSourcePath += "@" + m_sRev.value();
        }
    }

    hyprload::Result<std::monostate, std::string> GitPluginSource::installSource() {
        std::string command = "git clone " + m_sUrl + " " + m_pSourcePath.string();

        if (m_sBranch.has_value()) {
            command += " --branch " + m_sBranch.value();
        }

        if (std::system(command.c_str()) != 0) {
            return hyprload::Result<std::monostate, std::string>::err(
                "Failed to clone plugin source");
        }

        if (m_sRev.has_value()) {
            command = "git -C " + m_pSourcePath.string() + " checkout " + m_sRev.value();

            if (std::system(command.c_str()) != 0) {
                return hyprload::Result<std::monostate, std::string>::err(
                    "Failed to checkout revision");
            }
        }

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    bool GitPluginSource::isSourceAvailable() {
        return std::filesystem::exists(m_pSourcePath / ".git");
    }

    bool GitPluginSource::isUpToDate() {
        if (m_sRev.has_value()) {
            std::string command = "git -C " + m_pSourcePath.string() + " rev-parse HEAD";

            auto [exit, output] = executeCommand(command);

            if (exit != 0) {
                return false;
            }

            return output == m_sRev.value();
        }

        std::string command = "git -C " + m_pSourcePath.string() + " remote update";

        if (std::system(command.c_str()) != 0) {
            return false;
        }

        command = "git -C " + m_pSourcePath.string() + " status -uno";

        auto [exit, output] = executeCommand(command);

        return exit != 0 && output.find("behind") == std::string::npos;
    }

    bool GitPluginSource::providesPlugin(const std::string& name) const {
        auto pluginManifest = getPluginManifest(m_pSourcePath, name);

        if (pluginManifest.isErr()) {
            return false;
        }

        return true;
    }

    hyprload::Result<std::monostate, std::string> GitPluginSource::update(const std::string& name) {
        if (m_sRev.has_value()) {
            std::string command =
                "git -C " + m_pSourcePath.string() + " checkout " + m_sRev.value();

            if (std::system(command.c_str()) != 0) {
                return hyprload::Result<std::monostate, std::string>::err(
                    "Failed to checkout revision");
            }

            return this->install(name);
        }

        if (m_sBranch.has_value()) {
            std::string command =
                "git -C " + m_pSourcePath.string() + " checkout " + m_sBranch.value();

            if (std::system(command.c_str()) != 0) {
                return hyprload::Result<std::monostate, std::string>::err(
                    "Failed to checkout branch");
            }
        }

        std::string command = "git -C " + m_pSourcePath.string() + " pull";

        if (std::system(command.c_str()) != 0) {
            return hyprload::Result<std::monostate, std::string>::err(
                "Failed to update plugin source");
        }

        return this->install(name);
    }

    hyprload::Result<std::monostate, std::string>
    GitPluginSource::install(const std::string& name) {
        if (!this->isSourceAvailable()) {
            return this->installSource();
        }

        auto result = build(name);

        if (result.isErr()) {
            return result;
        }

        auto pluginManifestResult = getPluginManifest(m_pSourcePath, name);

        if (pluginManifestResult.isErr()) {
            return hyprload::Result<std::monostate, std::string>::err(
                pluginManifestResult.unwrapErr());
        }

        auto pluginManifest = pluginManifestResult.unwrap();

        std::filesystem::path outputBinary = m_pSourcePath / pluginManifest.getBinaryOutputPath();

        if (!std::filesystem::exists(outputBinary)) {
            return hyprload::Result<std::monostate, std::string>::err(
                "Plugin binary does not exist");
        }

        std::filesystem::path targetPath = hyprload::getPluginBinariesPath() / (name + ".so");

        if (std::filesystem::exists(targetPath)) {
            std::filesystem::remove(targetPath);
        }

        std::filesystem::copy(outputBinary, targetPath);

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    hyprload::Result<std::monostate, std::string> GitPluginSource::build(const std::string& name) {
        return buildPlugin(m_pSourcePath, name);
    }

    bool GitPluginSource::isEquivalent(const PluginSource& other) const {
        const auto& otherLocal = static_cast<const GitPluginSource&>(other);

        return m_sUrl == otherLocal.m_sUrl && m_sBranch == otherLocal.m_sBranch &&
            m_sRev == otherLocal.m_sRev && m_pSourcePath == otherLocal.m_pSourcePath;
    }

    LocalPluginSource::LocalPluginSource(std::filesystem::path&& path) : m_pSourcePath(path) {}

    hyprload::Result<std::monostate, std::string> LocalPluginSource::installSource() {
        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    bool LocalPluginSource::isSourceAvailable() {
        return std::filesystem::exists(m_pSourcePath);
    }

    bool LocalPluginSource::isUpToDate() {
        return false; // Always update local plugins, since they are not versioned
                      // and we don't know if they have changed.
    }

    bool LocalPluginSource::providesPlugin(const std::string& name) const {
        auto pluginManifest = getPluginManifest(m_pSourcePath, name);

        if (pluginManifest.isErr()) {
            return false;
        }

        return true;
    }

    hyprload::Result<std::monostate, std::string>
    LocalPluginSource::update(const std::string& name) {
        return this->install(name);
    }

    hyprload::Result<std::monostate, std::string>
    LocalPluginSource::install(const std::string& name) {
        if (!this->isSourceAvailable()) {
            return hyprload::Result<std::monostate, std::string>::err("Source for " + name +
                                                                      " does not exist");
        }

        auto result = build(name);

        if (result.isErr()) {
            return result;
        }

        auto pluginManifestResult = getPluginManifest(m_pSourcePath, name);

        if (pluginManifestResult.isErr()) {
            return hyprload::Result<std::monostate, std::string>::err(
                pluginManifestResult.unwrapErr());
        }

        const auto& pluginManifest = pluginManifestResult.unwrap();

        std::filesystem::path outputBinary = m_pSourcePath / pluginManifest.getBinaryOutputPath();

        if (!std::filesystem::exists(outputBinary)) {
            return hyprload::Result<std::monostate, std::string>::err(
                "Plugin binary does not exist");
        }

        std::filesystem::path targetPath = hyprload::getPluginBinariesPath() / (name + ".so");

        if (std::filesystem::exists(targetPath)) {
            std::filesystem::remove(targetPath);
        }

        std::filesystem::copy(outputBinary, targetPath);

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    hyprload::Result<std::monostate, std::string>
    LocalPluginSource::build(const std::string& name) {
        return buildPlugin(m_pSourcePath, name);
    }

    bool LocalPluginSource::isEquivalent(const PluginSource& other) const {
        const auto& otherLocal = static_cast<const LocalPluginSource&>(other);

        return m_pSourcePath == otherLocal.m_pSourcePath;
    }

    SelfSource::SelfSource() {}

    hyprload::Result<std::monostate, std::string> SelfSource::installSource() {
        std::string command = "git clone https://github.com/Duckonaut/hyprload.git " +
            getRootPath().string() + "/src";

        if (std::system(command.c_str()) != 0) {
            return hyprload::Result<std::monostate, std::string>::err("Failed to clone own source");
        }

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    bool SelfSource::isSourceAvailable() {
        return std::filesystem::exists(getRootPath() / "src" / ".git");
    }

    bool SelfSource::isUpToDate() {
        std::filesystem::path sourcePath = getRootPath() / "src";
        std::string command = "git -C " + sourcePath.string() + " remote update";

        if (std::system(command.c_str()) != 0) {
            return false;
        }

        command = "git -C " + sourcePath.string() + " status -uno";

        auto [exit, output] = executeCommand(command);

        return exit != 0 && output.find("behind") == std::string::npos;
    }

    bool SelfSource::providesPlugin(const std::string&) const {
        return false; // Don't provide any plugins.
    }

    hyprload::Result<std::monostate, std::string> SelfSource::update(const std::string& name) {
        std::string command = "git -C " + (getRootPath() / "src").string() + " pull";

        if (std::system(command.c_str()) != 0) {
            return hyprload::Result<std::monostate, std::string>::err(
                "Failed to update own source");
        }

        return this->install(name);
    }

    hyprload::Result<std::monostate, std::string> SelfSource::install(const std::string& name) {
        if (!this->isSourceAvailable()) {
            return this->installSource();
        }

        auto result = build(name);

        if (result.isErr()) {
            return result;
        }

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    hyprload::Result<std::monostate, std::string> SelfSource::build(const std::string&) {
        std::string buildSteps =
            "export HYPRLAND_COMMIT=" + g_pHyprload->getCurrentHyprlandCommitHash() +
            "export PKG_CONFIG_PATH=" + getPkgConfigOverridePath().string() + " && make -C " +
            (getRootPath() / "src").string() + " install";

        auto [exit, output] = executeCommand(buildSteps);

        if (exit != 0) {
            return hyprload::Result<std::monostate, std::string>::err("Failed to build self: " +
                                                                      output);
        }

        return hyprload::Result<std::monostate, std::string>::ok(std::monostate());
    }

    bool SelfSource::isEquivalent(const PluginSource&) const {
        return false; // Assume that the self source is different from everything else. It's
                      // probably not a good idea to have multiple self sources, ever.
    }

    PluginRequirement::PluginRequirement(const toml::table& plugin) {
        std::string source;

        if (plugin.contains("git") && plugin["git"].is_string()) {
            source = plugin["git"].as_string()->get();

            std::optional<std::string> branch = std::nullopt;

            if (plugin.contains("branch") && plugin["branch"].is_string()) {
                branch = plugin["branch"].as_string()->get();
            }

            std::optional<std::string> rev = std::nullopt;

            if (plugin.contains("rev") && plugin["rev"].is_string()) {
                rev = plugin["rev"].as_string()->get();
            }

            auto gitSource = std::make_shared<GitPluginSource>(std::string(source),
                                                               std::move(branch), std::move(rev));

            // Deduplicate sources
            bool found = false;
            for (const auto& pluginSource : g_vPluginSources) {
                if (*pluginSource == *gitSource) {
                    m_pSource = pluginSource;
                    found = true;
                    break;
                }
            }

            if (!found) {
                m_pSource = gitSource;
                g_vPluginSources.push_back(m_pSource);
            }
        } else if (plugin.contains("local") && plugin["local"].is_string()) {
            source = plugin["local"].as_string()->get();
            auto localSource = std::make_shared<LocalPluginSource>(std::filesystem::path(source));

            // Deduplicate sources
            bool found = false;
            for (const auto& pluginSource : g_vPluginSources) {
                if (*pluginSource == *localSource) {
                    m_pSource = pluginSource;
                    found = true;
                    break;
                }
            }

            if (!found) {
                m_pSource = localSource;
                g_vPluginSources.push_back(m_pSource);
            }
        } else {
            throw std::runtime_error("Plugin must have a source");
        }

        if (plugin.contains("name") && plugin["name"].is_string()) {
            m_sName = plugin["name"].as_string()->get();
        } else {
            m_sName = source.substr(source.find_last_of('/') + 1);
        }

        m_pBinaryPath = hyprload::getPluginBinariesPath() / (m_sName + ".so");
    }

    PluginRequirement::PluginRequirement(const std::string& plugin) {
        auto gitSource =
            std::make_shared<GitPluginSource>(std::string(plugin), std::nullopt, std::nullopt);

        // Deduplicate sources
        bool found = false;
        for (const auto& pluginSource : g_vPluginSources) {
            if (*pluginSource == *gitSource) {
                m_pSource = pluginSource;
                found = true;
                break;
            }
        }

        if (!found) {
            m_pSource = gitSource;
            g_vPluginSources.push_back(m_pSource);
        }

        m_sName = plugin.substr(plugin.find_last_of('/') + 1);

        m_pBinaryPath = hyprload::getPluginBinariesPath() / (m_sName + ".so");
    }

    const std::string& PluginRequirement::getName() const {
        return m_sName;
    }

    const std::filesystem::path& PluginRequirement::getBinaryPath() const {
        return m_pBinaryPath;
    }

    std::shared_ptr<PluginSource> PluginRequirement::getSource() const {
        return std::shared_ptr<PluginSource>(m_pSource);
    }

    bool PluginRequirement::isInstalled() const {
        return std::filesystem::exists(m_pBinaryPath);
    }
}
