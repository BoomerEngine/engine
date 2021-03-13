/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "instanceEditor.h"
#include "previewPanel.h"

#include "engine/material/include/materialInstance.h"

#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDataInspector.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialInstanceEditor);
RTTI_END_TYPE();

MaterialInstanceEditor::MaterialInstanceEditor(const ResourceInfo& info)
    : ResourceEditor(info, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo })
    , m_instance(rtti_cast<MaterialInstance>(info.resource))
{
    createInterface();

    m_previewPanel->bindMaterial(m_instance);
    m_properties->bindObject(m_instance);
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

//---

class MaterialInstaceResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstaceResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const override final
    {
        if (auto texture = rtti_cast<MaterialInstance>(context.resource))
        {
            outEditor = RefNew<MaterialInstanceEditor>(context);
            return true;
        }

        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialInstaceResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
