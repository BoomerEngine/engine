/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "base/editor/include/resourceEditor.h"
#include "base/editor/include/resourceEditorNativeFile.h"

namespace ed
{
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

        INLINE rendering::StaticTexturePtr texture() const { return rtti_cast<rendering::StaticTexture>(resource()); }

        //--

        virtual bool save() override;
        virtual void bindResource(const res::ResourcePtr& resource) override;

    private:
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

} // ed
