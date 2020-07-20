/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "managedFileFormat.h"
#include "managedFileNativeResource.h"
#include "resourceEditor.h"
#include "resourceEditorNativeFile.h"
#include "resourceEditorNativeImportAspect.h"
#include "assetFileImportWidget.h"

#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/resource_compiler/include/importInterface.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_CLASS(ResourceEditorNativeImportAspect);
    RTTI_END_TYPE();

    ResourceEditorNativeImportAspect::ResourceEditorNativeImportAspect()
    {
    }

    bool ResourceEditorNativeImportAspect::initialize(ResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        auto nativeFile = rtti_cast<ManagedFileNativeResource>(editor->file());
        if (!nativeFile)
            return false; // not a native file

        if (!editor->features().test(ResourceEditorFeatureBit::Imported))
            return false; // we don't want the importing business on this file

        const auto metadata = nativeFile->loadMetadata();
        if (!metadata || metadata->importDependencies.empty())
            return false; // file was not imported

        // get source import extension
        const auto& rootSourceAssetPath = metadata->importDependencies[0].importPath;
        const auto rootSourceFileExtension = rootSourceAssetPath.stringAfterLast(".");

        // get required config class, we need the source path to know what we will be importing
        SpecificClassType<res::ResourceConfiguration> configClass;
        res::IResourceImporter::ListImportConfigurationForExtension(rootSourceFileExtension, nativeFile->resourceClass(), configClass);
        if (!configClass)
            return false; // import is not configurable :(

        // build the base import config from source asset directory
        auto baseConfig = GetService<res::ImportFileService>()->compileBaseResourceConfiguration(rootSourceAssetPath, configClass);

        // apply the ASSET SPECIFIC config from a followup-import
        DEBUG_CHECK_EX(metadata->importBaseConfiguration, "No base configuration stored in metadata, very strange");
        if (metadata->importBaseConfiguration && metadata->importBaseConfiguration->is(configClass))
        {
            metadata->importBaseConfiguration->rebase(baseConfig);
            metadata->importBaseConfiguration->parent(nullptr);
            baseConfig = metadata->importBaseConfiguration;
        }

        // use the loaded user configuration
        DEBUG_CHECK_EX(metadata->importUserConfiguration, "No user configuration stored in metadata, very strange");
        DEBUG_CHECK_EX(!metadata->importUserConfiguration || metadata->importUserConfiguration->cls() == configClass, "User configuration stored in metadata has different class that currenyl recommended one");
        if (metadata->importUserConfiguration && metadata->importUserConfiguration->cls() == configClass)
        {
            m_config = metadata->importUserConfiguration;
            metadata->importUserConfiguration->parent(nullptr);
        }
        else
        {
            m_config = configClass.create();
        }

        //--

        {
            auto importPanel = base::CreateSharedPtr<ui::DockPanel>("[img:import] Import", "ImportSettings");
            importPanel->layoutVertical();

            {
                auto importWidget = importPanel->createChild<AssetFileImportWidget>();
                importWidget->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                importWidget->bindFile(nativeFile, m_config);
            }

            {
                auto properties = importPanel->createChild<ui::DataInspector>();
                properties->expand();
                properties->bindActionHistory(editor->actionHistory());
                properties->bindObject(m_config);
            }

            editor->dockLayout().right().attachPanel(importPanel, false);
        }

        //--

        return true;
    }

    void ResourceEditorNativeImportAspect::close()
    {
        TBaseClass::close();
        m_config.reset();
    }

    //--

} // editor

