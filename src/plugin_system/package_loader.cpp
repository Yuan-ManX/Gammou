
#include <fstream>
#include <nlohmann/json.hpp>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Linker/Linker.h>

#include "package_loader.h"
#include "node_widget_external_plugin.h"
#include "utils/serialization_helpers.h"

namespace Gammou {

    struct package_descriptor {
        std::string package_name{};
        package_uid uid;
        std::vector<node_widget_external_plugin::descriptor> plugins{};
        std::vector<std::filesystem::path> common_libs{};
        std::vector<package_uid> dependencies{};
    };

    void from_json(const nlohmann::json& j, package_descriptor& desc)
    {
        j.at("package-name").get_to(desc.package_name);
        j.at("package-uid").get_to(desc.uid);

        auto it = j.find("plugins");
        if (it != j.end())
            it->get_to(desc.plugins);

        it = j.find("common-libs");
        if (it != j.end())
            it->get_to(desc.common_libs);

        it = j.find("package-dependencies");
        if (it != j.end())
            it->get_to(desc.dependencies);
    }

    node_widget_factory_builder::node_widget_factory_builder(llvm::LLVMContext& llvm_context)
    :   _llvm_context{llvm_context}
    {
    }

    node_widget_factory_builder& node_widget_factory_builder::load_package(const std::filesystem::path& package_root_dir_path)
    {
        try
        {
            auto package = _load_package(package_root_dir_path);
            const auto uid = package.uid;
            _packages.emplace(uid, std::move(package));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[Package loader] Could not load package at '%s'\n%s\n",
                package_root_dir_path.generic_string().c_str(), e.what());
        }

        return *this;
    }

    node_widget_factory_builder& node_widget_factory_builder::load_packages(const std::filesystem::path& packages_dir_path)
    {
        LOG_INFO("[gammou][load package] Scanning package directory '%s'\n", packages_dir_path.generic_string().c_str());

        try {
            for (const auto& entry : std::filesystem::directory_iterator(packages_dir_path)) {
                const auto entry_path = entry.path();
                if (std::filesystem::is_directory(entry) && entry_path != ".." && entry_path != ".")
                    load_package(entry_path);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[gammou][load all packages] Unable to list directory package '%s'.\n%s\n",
                packages_dir_path.c_str(), e.what());
        }

        return *this;
    }

    std::unique_ptr<node_widget_factory> node_widget_factory_builder::build()
    {
        auto factory = std::make_unique<node_widget_factory>(_llvm_context);

        _resolve_dependencies();

        for (auto& pair : _packages) {
            auto& package = pair.second;
            for (auto&& plugin : package.loaded_plugins)
                factory->register_plugin(std::move(plugin));
            if (package.lib_module)
                factory->add_library_module(std::move(package.lib_module));
        }

        return factory;
    }

    void node_widget_factory_builder::_resolve_dependencies()
    {
        bool all_dependencies_satisfied = true;

        do {
            all_dependencies_satisfied = true;
            for (const auto& pair : _packages) {
                const auto& uid = pair.first;
                const auto& package = pair.second;
                bool dependencies_satisfied = true;

                for (const auto& dep : package.dependencies) {
                    if (_packages.find(dep) == _packages.end())
                    {
                        LOG_WARNING("[package loader] Unable to load package '%s', dependency uid:%lld is missing\n",
                            package.name, dep);
                        dependencies_satisfied = false;
                        break;
                    }
                }

                if (!dependencies_satisfied) {
                    // Remove package whose dependencies are not satisfied
                    _packages.erase(uid);
                    all_dependencies_satisfied = false;
                    break;
                }
            }
        } while (!all_dependencies_satisfied);
    }

    node_widget_factory_builder::package node_widget_factory_builder::_load_package(const std::filesystem::path& package_root_dir_path)
    {
        using namespace std::filesystem;
        LOG_DEBUG("[gammou][load package] Scanning package '%s'\n", package_root_dir_path.generic_string().c_str());

        //  a package is a directory
        if (!is_directory(package_root_dir_path))
            throw std::runtime_error("load_package : given path is not a directory");

        //  compute content file path
        auto content_file_path = package_root_dir_path / "content.json";
        nlohmann::json json_object;

        //  load json object from the file
        std::ifstream stream{ content_file_path };
        if (!stream.is_open())
            throw std::runtime_error("load_package : cannot load content.json file");
        else
            stream >> json_object;

        //  deserialize package descriptor from the file
        auto package_desc = json_object.get<package_descriptor>();
        package pack{
            package_desc.uid,
            package_desc.package_name,
            std::move(package_desc.dependencies)
        };

        LOG_INFO("[gammou][load package] Loading package uid : 0x%016llx, name : '%s'\n",
            package_desc.uid,
            package_desc.package_name.c_str());

        //  Load nodes plugins to factory
        for (auto& node_class_desc : package_desc.plugins) {

            //  Make sure that path are according to pwd
            for (auto& path : node_class_desc.modules_paths) {
                if (path.is_relative())
                    path = package_root_dir_path / path;
            }

            //  Create and plugin into factory
            LOG_DEBUG("[gammou][load package] Loading plugin uid : 0x%016lx, name : '%s'\n", node_class_desc.plugin_id, node_class_desc.name.c_str());

            try {
                pack.loaded_plugins.emplace_back(
                    node_widget_external_plugin::from_desc(
                        node_class_desc, _llvm_context));
            }
            catch (const std::exception& e)
            {
                // A plugin load failure is tolerable
                LOG_ERROR("[gammou][load package] Failed to load plugin '%s'.\n%s\n",
                    node_class_desc.name.c_str(), e.what());
            }
        }

        //  Load additional libs
        for (auto& lib_path : package_desc.common_libs) {
            llvm::SMDiagnostic error;

            if (lib_path.is_relative())
                lib_path = package_root_dir_path / lib_path;

            LOG_DEBUG("[gammou][load package] Loading common lib object %s\n",
                lib_path.generic_string().c_str());

            auto module = llvm::parseIRFile(lib_path.string(), error, _llvm_context);
            if (!module) {
                throw std::runtime_error("Failed to load common libs object");
            }
            else {
                //  Strip all function attribute as they prevent inlining.
                for (auto& function : *module)
                    function.setAttributes(
                        llvm::AttributeList::get(_llvm_context, llvm::ArrayRef<llvm::AttributeList>{}));
                if (pack.lib_module == nullptr)
                    pack.lib_module = std::move(module);
                else
                    llvm::Linker::linkModules(*pack.lib_module, std::move(module));
            }
        }

        return pack;
    }
}