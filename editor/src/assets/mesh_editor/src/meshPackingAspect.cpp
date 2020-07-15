/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#include "build.h"
#include "meshEditor.h"
#include "meshPackingAspect.h"

#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiButton.h"
#include "assets/mesh_loader/include/renderingMeshImportConfig.h"

namespace ed
{
    //---

/*    RTTI_BEGIN_TYPE_CLASS(MeshPackingAspect);
    RTTI_END_TYPE();

    MeshPackingAspect::MeshPackingAspect()
        : SingleResourceEditorImportConfigAspect(rendering::MeshImportConfig::GetStaticClass())
    {
    }

    bool MeshPackingAspect::initialize(SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto* meshEditor = base::rtti_cast<MeshEditor>(editor))
        {
            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:wrench_orange] Import");
            panel->layoutVertical();

            // the "reimport button"
            auto button = panel->createChild<ui::Button>("[img:arrow_refresh] Reimport from Soruce File");
            button->customMargins(10, 10, 10, 10);

            // properties
            m_properties = panel->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_properties->bindObject(config());

            meshEditor->dockLayout().right().attachPanel(panel);
            return true;
        }

        return false;
    }
    */
    //---

} // ed