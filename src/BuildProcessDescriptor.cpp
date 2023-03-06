#include "BuildProcessDescriptor.hpp"

namespace hyprload {
    BuildProcessDescriptor::BuildProcessDescriptor(
        std::string&& name, std::shared_ptr<hyprload::plugin::PluginSource> source,
        const std::filesystem::path& hyprlandHeadersPath) {
        m_sName = std::move(name);
        m_pSource = source;
        m_sHyprlandHeadersPath = hyprlandHeadersPath;
        m_rResult = std::nullopt;
    }
}
