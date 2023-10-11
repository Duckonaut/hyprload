#pragma once
#include "globals.hpp"
#include "toml/toml.hpp"
#include "types.hpp"

#include <memory>
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
        virtual bool providesPlugin(const std::string& name) const = 0;

        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string>
        update(const std::string& name) = 0;
        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string>
        build(const std::string& name) = 0;
        [[nodiscard]] virtual hyprload::Result<std::monostate, std::string>
        install(const std::string& name) = 0;

        bool operator==(const PluginSource& other) const;

      protected:
        virtual bool isEquivalent(const PluginSource& other) const = 0;
    };

    class GitPluginSource : public PluginSource {
      public:
        GitPluginSource(std::string&& url, std::optional<std::string>&& branch,
                        std::optional<std::string>&& rev);

        hyprload::Result<std::monostate, std::string> installSource() override;
        bool isSourceAvailable() override;
        bool isUpToDate() override;
        bool providesPlugin(const std::string& name) const override;

        hyprload::Result<std::monostate, std::string>
        update(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        build(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        install(const std::string& name) override;

      protected:
        bool isEquivalent(const PluginSource& other) const override;

      private:
        std::string m_sUrl;
        std::optional<std::string> m_sBranch;
        std::optional<std::string> m_sRev;
        std::filesystem::path m_pSourcePath;
    };

    class LocalPluginSource : public PluginSource {
      public:
        LocalPluginSource(std::filesystem::path&& source);

        hyprload::Result<std::monostate, std::string> installSource() override;
        bool isSourceAvailable() override;
        bool isUpToDate() override;
        bool providesPlugin(const std::string& name) const override;

        hyprload::Result<std::monostate, std::string>
        update(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        build(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        install(const std::string& name) override;

      protected:
        bool isEquivalent(const PluginSource& other) const override;

      private:
        std::filesystem::path m_pSourcePath;
    };

    class SelfSource : public PluginSource {
      public:
        SelfSource();

        hyprload::Result<std::monostate, std::string> installSource() override;
        bool isSourceAvailable() override;
        bool isUpToDate() override;
        bool providesPlugin(const std::string& name) const override;

        hyprload::Result<std::monostate, std::string>
        update(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        build(const std::string& name) override;
        hyprload::Result<std::monostate, std::string>
        install(const std::string& name) override;

      protected:
        bool isEquivalent(const PluginSource& other) const override;
    };

    class PluginRequirement {
      public:
        PluginRequirement(const toml::table& plugin);
        PluginRequirement(const std::string& plugin);

        const std::string& getName() const;
        const std::filesystem::path& getBinaryPath() const;

        std::shared_ptr<PluginSource> getSource() const;

        bool isInstalled() const;

      private:
        std::string m_sName;
        std::shared_ptr<PluginSource> m_pSource;
        std::filesystem::path m_pBinaryPath;
    };

    inline std::vector<std::shared_ptr<PluginSource>> g_vPluginSources;
}
