/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#include "build.h"
#include "fbxManifestAspect.h"

#include "assets/fbx_loader/include/fbxMeshCooker.h"
#include "assets/mesh_editor/include/meshEditor.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"

namespace fbx
{
    //---

    RTTI_BEGIN_TYPE_CLASS(FBXProcessingAspect);
    RTTI_END_TYPE();

    FBXProcessingAspect::FBXProcessingAspect()
        : SingleResourceEditorManifestAspect(fbx::MeshManifest::GetStaticClass())
    {
    }

    bool FBXProcessingAspect::initialize(ed::SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto* meshEditor = base::rtti_cast<ed::MeshEditor>(editor))
        {
            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:model] FBX");
            panel->layoutVertical();

            m_properties = panel->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_properties->bindObject(manifest());

            meshEditor->dockLayout().right().attachPanel(panel);
            return true;
        }

        return false;
    }

    //---

} // wavefront