#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "HyprloadPlugin.hpp"

namespace hyprload {
    class BuildProcessDescriptor final {
      public:
        BuildProcessDescriptor(std::string&& name,
                               std::shared_ptr<hyprload::plugin::PluginSource> source);

        std::string m_sName;
        std::shared_ptr<hyprload::plugin::PluginSource> m_pSource;

        std::mutex m_mMutex;
        std::optional<hyprload::Result<std::monostate, std::string>> m_rResult;
    };

}
