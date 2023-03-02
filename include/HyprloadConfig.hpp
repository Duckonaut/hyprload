#pragma once
#include "globals.hpp"
#include "toml/toml.hpp"
#include "HyprloadPlugin.hpp"

#include <string>
#include <filesystem>
#include <variant>
#include <vector>

namespace hyprload::config {
    const std::string c_pluginConfig = "plugin:hyprload:config";

    std::filesystem::path getConfigPath();


    class HyprloadConfig {
      public:
        HyprloadConfig();

        const toml::table& getConfig() const;
        const std::vector<hyprload::plugin::PluginRequirement>& getPlugins() const;

      private:
        std::unique_ptr<toml::table> m_pConfig;
        std::vector<hyprload::plugin::PluginRequirement> m_vPluginsWanted;
    };

    inline std::unique_ptr<HyprloadConfig> g_pHyprloadConfig;
}
