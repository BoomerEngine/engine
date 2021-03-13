/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"

#include "resourceEditor.h"
#include "resourceReimport.h"
#include "importWidget.h"

#include "editor/common/include/service.h"
#include "editor/common/include/utils.h"

#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiDockLayout.h"

#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/importInterface.h"
#include "core/resource_compiler/include/sourceAssetService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ResourceReimportPanel);
RTTI_END_TYPE();

ResourceReimportPanel::ResourceReimportPanel(ResourceEditor* editor)
    : ui::DockPanel("[img:import] Import", "ImportSettings")
    , m_editor(editor)
{
    layoutVertical();

    {
        m_importWidget = createChild<AssetFileImportWidget>();
        m_importWidget->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_importWidget->bindFile(editor->info().metadataDepotPath, m_userConfig);
        m_importWidget->bind(EVENT_RESOURCE_REIMPORT_WITH_CONFIG) = [this](ResourceConfigurationPtr config)
        {
            inplaceReimport();
        };
    }

    {
        m_dataInspector = createChild<ui::DataInspector>();
        m_dataInspector->expand();
        m_dataInspector->bindActionHistory(editor->actionHistory());
    }

    updateMetadata(editor->info().metadata);
}

//--

void ResourceReimportPanel::updateMetadata(ResourceMetadata* metadata)
{
    // get source import extension
    const auto& rootSourceAssetPath = metadata->importDependencies[0].importPath;
    const auto rootSourceFileExtension = rootSourceAssetPath.stringAfterLast(".");

    // get required config class, we need the source path to know what we will be importing
    SpecificClassType<ResourceConfiguration> configClass;
    IResourceImporter::ListImportConfigurationForExtension(rootSourceFileExtension, metadata->resourceClassType, configClass);

    // build the base import config from source asset directory
    m_mergedBaseConfig = configClass->create<ResourceConfiguration>();
    GetService<SourceAssetService>()->collectImportConfiguration(m_mergedBaseConfig, rootSourceAssetPath);

    // apply the ASSET SPECIFIC config from a followup-import
    if (metadata->importBaseConfiguration && metadata->importBaseConfiguration->is(configClass))
    {
        auto clonedBase = CloneObject(metadata->importBaseConfiguration);
        clonedBase->rebase(m_mergedBaseConfig);
        clonedBase->detach();
        m_mergedBaseConfig = clonedBase;
    }

    // rebase the config on the BASE config stuff
    m_userConfig = metadata->importUserConfiguration;
    m_userConfig->rebase(m_mergedBaseConfig);

    // show the user config
    m_dataInspector->bindObject(m_userConfig);
}

void ResourceReimportPanel::inplaceReimport()
{
    RunLongAction(this, nullptr, [this](IProgressTracker& progress)
        {
            const auto depotPath = m_editor->info().resourceDepotPath;

            ResourcePtr newResource;
            ResourceMetadataPtr newMetadata;
            if (ReimportResource(depotPath, m_editor->info().resource, m_editor->info().metadata, newResource, newMetadata, &progress))
            {
                if (!progress.checkCancelation())
                {
                    auto editor = m_editor;
                    editor->runSync([newResource, newMetadata, editor]()
                        {
                            editor->handleReimportAction(newResource, newMetadata);
                        });
                }
            }
        }, "Reimporting resource...", true);
}

//--

END_BOOMER_NAMESPACE_EX(ed)


