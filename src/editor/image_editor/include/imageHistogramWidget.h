/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/canvas/include/geometry.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct ImageHistogramData;

//--

/// helper widget that shows the image histogram
class EDITOR_IMAGE_EDITOR_API ImageHistogramWidget : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImageHistogramWidget, IElement);

public:
    ImageHistogramWidget();

    void addHistogram(const RefPtr<ImageHistogramData>& data, Color color, StringView caption);
    void removeHistograms();

protected:
    virtual void renderBackground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;
    virtual void renderForeground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;
    virtual bool handleMouseMovement(const input::MouseMovementEvent& evt) override;
    virtual void handleHoverLeave(const ui::Position& absolutePosition) override;

    virtual ui::ElementPtr queryTooltipElement(const ui::Position& absolutePosition, ui::ElementArea& outArea) const override;

    struct CollapsedBucket
    {
        uint32_t bucketCount = 0;
        uint32_t bucketValue = 0;
    };

    struct Histogram
    {
        RefPtr<ImageHistogramData> data;
        Color color;
        StringView caption;
        Array<CollapsedBucket> collapsedBuckets;
        uint32_t collapsedBucketsMaxValue = 0;
        canvas::Geometry geometry;
    };

    double m_histogramMin = 0.0;
    double m_histogramMax = 1.0;
    uint32_t m_histogramBucketMax = 0;

    Array<Histogram> m_histograms;

    ui::Size m_cachedHistogramGeometryRefSize;

    mutable RefWeakPtr<ui::TextLabel> m_activeTooltip;

    int m_hoverPositionX = -1;

    void collapseHistogramBuckets(Histogram& h);
    void cacheHistogramGeometry();
    void updateTooltip() const;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
