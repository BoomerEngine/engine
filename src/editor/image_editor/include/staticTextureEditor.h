/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "editor/assets/include/resourceEditor.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class ImageHistogramWidget;
class ImagePreviewPanelWithToolbar;
class ImageCubePreviewPanelWithToolbar;
class ImageHistogramPendingData;

/// editor for static textures (baked)
class StaticTextureEditor : public ResourceEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureEditor, ResourceEditor);

public:
    StaticTextureEditor(const ResourceInfo& info, IStaticTexture* texture);
    virtual ~StaticTextureEditor();

    INLINE ImagePreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }

    INLINE const StaticTexturePtr& texture() const { return m_texture; }

    //--

    virtual void reimported(ResourcePtr resource, ResourceMetadataPtr metadata) override;

private:
    StaticTexturePtr m_texture;

    ImageHistogramWidget* m_colorHistogram = nullptr;
    ImageHistogramWidget* m_lumHistogram = nullptr;

    ImagePreviewPanelWithToolbar* m_previewPanel = nullptr;
    ImageCubePreviewPanelWithToolbar* m_cubePreviewPanel = nullptr;

    ui::TextLabel* m_imageInfoLabel;

    Array<RefPtr<ImageHistogramPendingData>> m_pendingHistograms;
    ui::Timer m_histogramCheckTimer;

    void createInterface();
    void updateImageInfoText();

    void updateHistogram();
    void checkHistograms();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
