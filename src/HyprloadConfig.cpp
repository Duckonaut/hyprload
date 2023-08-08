#include "util.hpp"

#include "HyprloadConfig.hpp"
#include "Hyprload.hpp"
#include "HyprloadPlugin.hpp"

#include "toml/toml.hpp"

#include <cstddef>
#include <hyprland/src/config/ConfigManager.hpp>

namespace hyprload::config {
    std::filesystem::path getConfigPath() {
        static SConfigValue* hyprloadConfig = HyprlandAPI::getConfigValue(PHANDLE, c_pluginConfig);

        return std::filesystem::path(hyprloadConfig->strValue);
    }

    HyprloadConfig::HyprloadConfig() {
        try {
            m_pConfig = std::make_unique<toml::table>(toml::parse_file(getConfigPath().u8string()));
        } catch (const std::exception& e) {
            const std::string error = e.what();
            hyprload::error("Failed to parse config file: " + error);
            return;
        }

        if (m_pConfig->contains("plugins") && m_pConfig->get("plugins")->is_array()) {
            m_pConfig->get("plugins")->as_array()->for_each(
                [&plugins = m_vPluginsWanted](const toml::node& value) {
                    if (value.is_string()) {
                        plugins.emplace_back(value.as_string()->get());
                    } else if (value.is_table()) {
                        try {
                            plugins.emplace_back(*value.as_table());
                        } catch (const std::exception& e) {
                            const std::string error = e.what();
                            hyprload::error("Failed to parse plugin: " + error);
                        }
                    } else {
                        hyprload::error("Plugin must be a string or table");
                    }
                });
        }

    }

    void HyprloadConfig::reloadConfig() {
        m_pConfig = nullptr;
        m_vPluginsWanted.clear();

        try {
            m_pConfig = std::make_unique<toml::table>(toml::parse_file(getConfigPath().u8string()));
        } catch (const std::exception& e) {
            const std::string error = e.what();
            hyprload::error("Failed to parse config file: " + error);
            return;
        }

        if (m_pConfig->contains("plugins") && m_pConfig->get("plugins")->is_array()) {
            m_pConfig->get("plugins")->as_array()->for_each(
                [&plugins = m_vPluginsWanted](const toml::node& value) {
                    if (value.is_string()) {
                        plugins.emplace_back(value.as_string()->get());
                    } else if (value.is_table()) {
                        try {
                            plugins.emplace_back(*value.as_table());
                        } catch (const std::exception& e) {
                            const std::string error = e.what();
                            hyprload::error("Failed to parse plugin: " + error);
                        }
                    } else {
                        hyprload::error("Plugin must be a string or table");
                    }
                });
        }
    }

    const toml::table& HyprloadConfig::getConfig() const {
        return *m_pConfig;
    }

    const std::vector<hyprload::plugin::PluginRequirement>& HyprloadConfig::getPlugins() const {
        return m_vPluginsWanted;
    }
}
