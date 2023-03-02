#pragma once
#include "globals.hpp"
#include "toml/toml.hpp"
#include "types.hpp"

#include <string>
#include <filesystem>
#include <variant>
#include <vector>

namespace hyprload::plugin {
    class PluginManifest {
      public:
        PluginManifest(std::string&& name, const toml::table& manifest);

        const std::string& getName() const;
        const std::vector<std::string>& getAuthors() const;
        const std::string& getVersion() const;
        const std::string& getDescription() const;

        const std::filesystem::path& getBinaryOutputPath() const;
        const std::vector<std::string>& getBuildSteps() const;

      private:
        std::string m_sName;
        std::vector<std::string> m_sAuthors;
        std::string m_sVersion;
        std::string m_sDescription;

        std::filesystem::path m_pBinaryOutputPath;
        std::vector<std::string> m_sBuildSteps;
    };

    class HyprloadManifest {
      public:
        HyprloadManifest(const toml::table& manifest);

        const std::vector<PluginManifest>& getPlugins() const;

      private:
        std::vector<PluginManifest> m_vPlugins;
    };

    class PluginSource {
      public:
        virtual ~PluginSource() = default;

        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string> installSource() = 0;
        virtual bool isSourceAvailable() = 0;
        virtual bool isUpToDate() = 0;

        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string> update(const std::string& name) = 0;
        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string> build(const std::string& name) = 0;
        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string> install(const std::string& name) = 0;

        bool operator==(const PluginSource& other) const;
      protected:
        virtual bool isEquivalent(const PluginSource& other) const = 0;
    };

    class GitPluginSource : public PluginSource {
      public:
        GitPluginSource(std::string&& url, std::string&& branch);

        hyprload::Result<std::monostate, std::string> installSource() override;
        bool isSourceAvailable() override;
        bool isUpToDate() override;

        hyprload::Result<std::monostate, std::string> update(const std::string& name) override;
        hyprload::Result<std::monostate, std::string> build(const std::string& name) override;
        hyprload::Result<std::monostate, std::string> install(const std::string& name) override;

      protected:
        bool isEquivalent(const PluginSource& other) const override;

      private:
        std::string m_sUrl;
        std::string m_sBranch;
        std::filesystem::path m_pSourcePath;
    };

    class LocalPluginSource : public PluginSource {
      public:
        LocalPluginSource(std::filesystem::path&& source);

        hyprload::Result<std::monostate, std::string> installSource() override;
        bool isSourceAvailable() override;
        bool isUpToDate() override;

        hyprload::Result<std::monostate, std::string> update(const std::string& name) override;
        hyprload::Result<std::monostate, std::string> build(const std::string& name) override;
        hyprload::Result<std::monostate, std::string> install(const std::string& name) override;

      protected:
        bool isEquivalent(const PluginSource& other) const override;

      private:
        std::filesystem::path m_pSourcePath;
    };

    class PluginDescription {
      public:
        PluginDescription(const toml::table& plugin);
        PluginDescription(const std::string& plugin);

        const std::string& getName() const;
        const PluginSource& getSource() const;
        const std::filesystem::path& getSourcePath() const;
        const std::filesystem::path& getBinaryPath() const;

      private:
        std::string m_sName;
        std::shared_ptr<PluginSource> m_pSource;
        std::filesystem::path m_pSourcePath;
        std::filesystem::path m_pBinaryPath;
    };
}
