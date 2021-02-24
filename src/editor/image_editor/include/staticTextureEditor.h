/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "editor/common/include/resourceEditor.h"
#include "editor/common/include/resourceEditorNativeFile.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

class ImageHistogramWidget;
class ImagePreviewPanelWithToolbar;
class ImageHistogramPendingData;

/// editor for static textures (baked)
class StaticTextureEditor : public ResourceEditorNativeFile
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureEditor, ResourceEditorNativeFile);

public:
    StaticTextureEditor(ManagedFileNativeResource* file);
    virtual ~StaticTextureEditor();

    INLINE ImagePreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }

    INLINE const rendering::StaticTexturePtr& texture() const { return m_texture; }

    //--

    virtual bool initialize() override;
    virtual void handleLocalReimport(const res::ResourcePtr& ptr) override;

private:
    rendering::StaticTexturePtr m_texture;

    ImageHistogramWidget* m_colorHistogram;
    ImageHistogramWidget* m_lumHistogram;

    ui::DockPanel* m_previewTab;
    ImagePreviewPanelWithToolbar* m_previewPanel;

    ui::TextLabel* m_imageInfoLabel;

    base::Array<base::RefPtr<ImageHistogramPendingData>> m_pendingHistograms;
    ui::Timer m_histogramCheckTimer;

    void createInterface();
    void updateImageInfoText();

    void updateHistogram();
    void checkHistograms();
};

//--

END_BOOMER_NAMESPACE(ed)