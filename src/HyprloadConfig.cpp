#include "HyprloadConfig.hpp"
#include "Hyprload.hpp"
#include "toml/toml.hpp"

#include <src/config/ConfigManager.hpp>

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
            hyprload::log("Failed to parse config file: " + error);
        }

        if (m_pConfig->contains("plugins") && m_pConfig->get("plugins")->is_array()) {
            m_pConfig->get("plugins")->as_array()->for_each([&plugins = m_vPlugins](const toml::node& value) {
                if (value.is_string()) {
                    plugins.emplace_back(value.as_string()->get());
                } else if (value.is_table()) {
                    try {
                        plugins.emplace_back(*value.as_table());
                    } catch (const std::exception& e) {
                        const std::string error = e.what();
                        hyprload::log("Failed to parse plugin: " + error);
                    }
                } else {
                    hyprload::log("Plugin must be a string or table");
                }
            });
        }
    }

    const toml::table& HyprloadConfig::getConfig() const {
        return *m_pConfig;
    }

    const std::vector<hyprload::plugin::PluginDescription>& HyprloadConfig::getPlugins() const {
        return m_vPlugins;
    }
}
