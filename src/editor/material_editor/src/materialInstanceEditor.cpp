/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialInstanceEditor.h"
#include "materialPreviewPanel.h"

#include "engine/material/include/materialInstance.h"

#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDataInspector.h"
#include "editor/common/include/assetBrowser.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedFileNativeResource.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialInstanceEditor);
RTTI_END_TYPE();

MaterialInstanceEditor::MaterialInstanceEditor(ManagedFileNativeResource* file)
    : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo, ResourceEditorFeatureBit::Imported })
{
    createInterface();
}

MaterialInstanceEditor::~MaterialInstanceEditor()
{}

void MaterialInstanceEditor::createInterface()
{
    {
        auto tab = RefNew<ui::DockPanel>("[img:shader] Preview", "PreviewPanel");
        tab->layoutVertical();

        m_previewPanel = tab->createChild<MaterialPreviewPanel>();
        m_previewPanel->expand();

        dockLayout().attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:properties] Properties", "PropertiesPanel");
        tab->layoutVertical();

        m_properties = tab->createChild<ui::DataInspector>();
        m_properties->bindActionHistory(actionHistory());
        m_properties->expand();

        dockLayout().right(0.2f).attachPanel(tab);
    }
}

bool MaterialInstanceEditor::save()
{
    if (!TBaseClass::save())
        return false;

    LoadResource<MaterialInstance>(file()->depotPath());
    return true;
}

bool MaterialInstanceEditor::initialize()
{
    if (!TBaseClass::initialize())
        return false;

    m_instance = rtti_cast<MaterialInstance>(resource());
    if (!m_instance)
        return false;

    m_previewPanel->bindMaterial(m_instance);
    m_properties->bindData(m_instance->createDataView());
    return true;
}

//---

class MaterialInstaceResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstaceResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool canOpen(const ManagedFileFormat& format) const override
    {
        const auto graphClass = MaterialInstance::GetStaticClass();
        return (format.nativeResourceClass() == graphClass);
    }

    virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            return RefNew<MaterialInstanceEditor>(nativeFile);
                
        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialInstaceResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
