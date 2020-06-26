/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshPreviewPanelWithToolbar.h"

#include "base/ui/include/uiToolBar.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_CLASS(MeshPreviewPanelWithToolbar);
    RTTI_END_TYPE();

    MeshPreviewPanelWithToolbar::MeshPreviewPanelWithToolbar()
    {
        layoutVertical();

        actions().bindCommand("MeshPreview.ShowBounds"_id) = [this]() { auto settings = previewSettings(); settings.showBounds = !settings.showBounds; previewSettings(settings); };
        actions().bindToggle("MeshPreview.ShowBounds"_id) = [this]() { return previewSettings().showBounds; };

        {
            m_toolbar = createChild<ui::ToolBar>();
            m_toolbar->createButton("MeshPreview.ShowBounds"_id, "[img:bracket]", "Show mesh bounds");
            m_toolbar->createSeparator();
        }

        {
            m_previewPanel = createChild<MeshPreviewPanel>();
            m_previewPanel->buildDefaultToolbar(this, m_toolbar);
        }
    }

    MeshPreviewPanelWithToolbar::~MeshPreviewPanelWithToolbar()
    {
    }

    const MeshPreviewPanelSettings& MeshPreviewPanelWithToolbar::previewSettings() const
    {
        return m_previewPanel->previewSettings();
    }

    void MeshPreviewPanelWithToolbar::previewMaterial(base::StringID name, const rendering::MaterialPtr& data)
    {
        m_previewPanel->previewMaterial(name, data);
    }

    void MeshPreviewPanelWithToolbar::previewMesh(const rendering::MeshPtr& mesh)
    {
        if (mesh != m_mesh)
        {
            m_mesh = mesh;
            m_previewPanel->previewMesh(mesh);
        }
    }

    void MeshPreviewPanelWithToolbar::previewSettings(const MeshPreviewPanelSettings& settings)
    {
        m_previewPanel->previewSettings(settings);
    }

    //--
    
} // ed
