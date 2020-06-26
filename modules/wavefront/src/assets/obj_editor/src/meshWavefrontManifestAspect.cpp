/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#include "build.h"
#include "meshWavefrontManifestAspect.h"

#include "assets/obj_loader/include/wavefrontMeshCooker.h"
#include "assets/mesh_editor/include/meshEditor.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"

namespace wavefront
{
    //---

    RTTI_BEGIN_TYPE_CLASS(WavefrontProcessingAspect);
    RTTI_END_TYPE();

    WavefrontProcessingAspect::WavefrontProcessingAspect()
        : SingleResourceEditorManifestAspect(wavefront::MeshManifest::GetStaticClass())
    {
    }

    bool WavefrontProcessingAspect::initialize(ed::SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto* meshEditor = base::rtti_cast<ed::MeshEditor>(editor))
        {
            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:model] Wavefront");
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