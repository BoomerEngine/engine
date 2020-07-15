/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticTextureEditor.h"
#include "staticTextureCompressionAspect.h"

#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"
#include "assets/image_texture/include/staticTextureImporter.h"
#include "base/ui/include/uiButton.h"

namespace ed
{
    //---

    /*RTTI_BEGIN_TYPE_CLASS(StaticTextureCompressionAspect);
    RTTI_END_TYPE();

    StaticTextureCompressionAspect::StaticTextureCompressionAspect()
        : SingleResourceEditorImportConfigAspect(rendering::StaticTextureCompressionConfiguration::GetStaticClass())
    {
    }

    bool StaticTextureCompressionAspect::initialize(SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto* textureEditor = base::rtti_cast<StaticTextureEditor>(editor))
        {
            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:folder_zip] Compression");
            panel->layoutVertical();

            // the "reimport button"
            auto button = panel->createChild<ui::Button>("[img:arrow_refresh] Reimport from Soruce File");
            button->customMargins(10, 10, 10, 10);

            // properties
            m_properties = panel->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_properties->bindObject(config());

            textureEditor->dockLayout().right().attachPanel(panel, true);
            return true;
        }

        return false;
    }
    */
    //---

} // ed