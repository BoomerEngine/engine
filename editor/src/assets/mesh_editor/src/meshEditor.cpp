/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshEditor.h"
#include "meshPreviewPanel.h"
#include "meshPreviewPanelWithToolbar.h"

#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/ui/include/uiDockPanel.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiRuler.h"
#include "base/ui/include/uiSplitter.h"
#include "base/geometry/include/mesh.h"
#include "meshStructurePanel.h"

#include "rendering/mesh/include/renderingMesh.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshEditor);
    RTTI_END_TYPE();

    MeshEditor::MeshEditor(ConfigGroup config, ManagedFile* file)
        : SingleCookedResourceEditor(config, file, rendering::Mesh::GetStaticClass())
    {
        createInterface();
    }

    MeshEditor::~MeshEditor()
    {}

    void MeshEditor::createInterface()
    {
        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:world] Preview");
            tab->layoutVertical();

            m_previewPanel = tab->createChild<MeshPreviewPanelWithToolbar>();
            m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:tree] Structure");
            tab->layoutVertical();

            m_structurePanel = tab->createChild<MeshStructurePanel>();
            m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

            dockLayout().right().attachPanel(tab);
        }

        previewResourceChanged();
    }

    void MeshEditor::previewResourceChanged()
    {
        TBaseClass::previewResourceChanged();

        if (auto mesh = base::rtti_cast<rendering::Mesh>(previewResource().acquire()))
        {
            m_previewPanel->previewMesh(mesh);
        }
    }

    //---

    class MeshResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            const auto meshClass = rendering::Mesh::GetStaticClass();
            for (const auto& output : format.cookableOutputs())
                if (output.resoureClass == meshClass)
                    return true;

            return false;
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const override
        {
            return base::CreateSharedPtr<MeshEditor>(config, file);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MeshResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed