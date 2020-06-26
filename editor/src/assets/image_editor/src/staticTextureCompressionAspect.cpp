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
#include "assets/image_texture/include/staticTextureCooker.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_CLASS(StaticTextureCompressionAspect);
    RTTI_END_TYPE();

    StaticTextureCompressionAspect::StaticTextureCompressionAspect()
        : SingleResourceEditorManifestAspect(rendering::StaticTextureFromImageManifest::GetStaticClass())
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

            m_properties = panel->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_properties->bindObject(manifest());

            textureEditor->dockLayout().right().attachPanel(panel, true);
            return true;
        }

        return false;
    }

    //---

} // ed