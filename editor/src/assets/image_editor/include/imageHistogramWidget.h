/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/ui/include/uiElement.h"

namespace ed
{

    //--

    struct ImageHistogramData;

    //--

    /// helper widget that shows the image histogram
    class ASSETS_IMAGE_EDITOR_API ImageHistogramWidget : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ImageHistogramWidget, IElement);

    public:
        ImageHistogramWidget();

        void addHistogram(const base::RefPtr<ImageHistogramData>& data, base::Color color, base::StringView<char> caption);
        void removeHistograms();

    protected:
        virtual void renderBackground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual void renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual void handleHoverLeave(const ui::Position& absolutePosition) override;

        virtual ui::ElementPtr queryTooltipElement(const ui::Position& absolutePosition) const override;

        struct CollapsedBucket
        {
            uint32_t bucketCount = 0;
            uint32_t bucketValue = 0;
        };

        struct Histogram
        {
            base::RefPtr<ImageHistogramData> data;
            base::Color color;
            base::StringView<char> caption;
            base::Array<CollapsedBucket> collapsedBuckets;
            uint32_t collapsedBucketsMaxValue = 0;
            base::RefPtr<base::canvas::Geometry> geometry;
        };

        double m_histogramMin = 0.0;
        double m_histogramMax = 1.0;
        uint32_t m_histogramBucketMax = 0;

        base::Array<Histogram> m_histograms;

        ui::Size m_cachedHistogramGeometryRefSize;

        mutable base::RefWeakPtr<ui::TextLabel> m_activeTooltip;

        int m_hoverPositionX = -1;

        void collapseHistogramBuckets(Histogram& h);
        void cacheHistogramGeometry();
        void updateTooltip() const;
    };

    //--

} // ed