/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imageHistogramWidget.h"
#include "imageHistogramCalculation.h"
#include "core/image/include/imageView.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

void ComputeHistogramUint8(uint8_t channel, const image::ImageView& view, ImageHistogramData& outData)
{
    for (image::ImageViewSliceIterator z(view); z; ++z)
    {
        for (image::ImageViewRowIterator y(z); y; ++y)
        {
            for (image::ImageViewPixelIterator x(y); x; ++x)
            {
                auto val = x.data()[channel];
                outData.buckets[val] += 1;
            }
        }
    }
}

void ComputeHistogramUint16(uint8_t channel, const image::ImageView& view, ImageHistogramData& outData)
{
    for (image::ImageViewSliceIterator z(view); z; ++z)
    {
        for (image::ImageViewRowIterator y(z); y; ++y)
        {
            for (image::ImageViewPixelIterator x(y); x; ++x)
            {
                auto val = ((const uint16_t*)x.data())[channel];
                outData.buckets[val] += 1;
            }
        }
    }
}

RefPtr<ImageHistogramData> ComputeHistogram(uint8_t channel, const image::ImageView& view)
{
    if (channel >= view.channels())
        return nullptr;

    if (view.format() == image::PixelFormat::Uint8_Norm)
    {
        auto ret = RefNew<ImageHistogramData>();
        ret->minValue = 0.0f;
        ret->maxValue = 255.0f;
        ret->totalPixelCount = view.width() * view.height() * view.depth();
        ret->buckets.resizeWith(256, 0);

        ComputeHistogramUint8(channel, view, *ret);

        for (const auto val : ret->buckets)
            ret->maxBucketValue = std::max<uint32_t>(ret->maxBucketValue, val);

        return ret;
    }
    else if (view.format() == image::PixelFormat::Uint16_Norm)
    {
        auto ret = RefNew<ImageHistogramData>();
        ret->minValue = 0.0f;
        ret->maxValue = 65535.0f;
        ret->totalPixelCount = view.width() * view.height() * view.depth();
        ret->buckets.resizeWith(65536.0f, 0);

        ComputeHistogramUint8(channel, view, *ret);

        for (const auto val : ret->buckets)
            ret->maxBucketValue = std::max<uint32_t>(ret->maxBucketValue, val);

        return ret;
    }
        

    return nullptr;
}

//---

RTTI_BEGIN_TYPE_CLASS(ImageHistogramWidget);
RTTI_END_TYPE();

ImageHistogramWidget::ImageHistogramWidget()
{
    hitTest(true);
    enableAutoExpand(true, false);
    customMinSize(0, 250);
}

void ImageHistogramWidget::addHistogram(const RefPtr<ImageHistogramData>& data, Color color, StringView caption)
{
    if (data)
    {
        bool added = false;

        for (auto& entry : m_histograms)
        {
            if (entry.caption == caption)
            {
                entry.data = data;
                added = true;
                break;
            }
        }

        if (!added)
        {
            auto& entry = m_histograms.emplaceBack();
            entry.data = data;
            entry.color = color;
            entry.caption = caption;
        }

        m_histogramMin = std::min<double>(m_histogramMin, data->minValue);
        m_histogramMax = std::max<double>(m_histogramMax, data->maxValue);
        m_histogramBucketMax = std::max<uint32_t>(m_histogramBucketMax, data->maxBucketValue);

        m_cachedHistogramGeometryRefSize = ui::Size(0, 0);
    }
}

void ImageHistogramWidget::removeHistograms()
{
    m_histograms.clear();
    m_histogramMin = 0.0;
    m_histogramMax = 1.0;
    m_histogramBucketMax = 1;
}

void ImageHistogramWidget::renderBackground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);
}

void ImageHistogramWidget::collapseHistogramBuckets(Histogram& h)
{
    uint32_t optimalBucketCount = h.data->buckets.size();

    auto width = optimalBucketCount;// std::max<uint32_t>((uint32_t)m_cachedHistogramGeometryRefSize.x, optimalBucketCount);

    h.collapsedBuckets.resize(width);
    h.collapsedBucketsMaxValue = 1;

    const double posStep = (m_histogramMax - m_histogramMin) / m_cachedHistogramGeometryRefSize.x;
    const double bucketSetp = width / (m_histogramMax - m_histogramMin);

    for (uint32_t i = 0; i < width; ++i)
    {
        auto& bucket = h.collapsedBuckets.typedData()[i];
        bucket.bucketCount = 0;
        bucket.bucketValue = 0;
    }

    const auto numBuckets = h.data->buckets.size();
    const auto bucketXStep = (h.data->maxValue - h.data->minValue) / (double)h.data->buckets.size();
    for (uint32_t i = 0; i < numBuckets; ++i)
    {
        const auto& bucket = h.data->buckets.typedData()[i];
        const auto x = h.data->minValue + i * bucketXStep;
        if (x >= m_histogramMin && x <= m_histogramMax)
        {
            auto collapsedBucketIndex = (int)((x - m_histogramMin) * bucketSetp);
            if (collapsedBucketIndex >= 0 && collapsedBucketIndex <= h.collapsedBuckets.lastValidIndex())
            {
                auto& collapsedBucket = h.collapsedBuckets.typedData()[collapsedBucketIndex];
                collapsedBucket.bucketCount += 1;
                collapsedBucket.bucketValue += bucket;
            }
        }
    }

    for (const auto& col : h.collapsedBuckets)
        h.collapsedBucketsMaxValue = std::max<uint32_t>(h.collapsedBucketsMaxValue, col.bucketValue);
}

void ImageHistogramWidget::cacheHistogramGeometry()
{
    const auto width = m_cachedHistogramGeometryRefSize.x;
    const auto height = m_cachedHistogramGeometryRefSize.y;

    uint32_t maxBucketValueEver = 1;
    for (auto& hist : m_histograms)
    {
        collapseHistogramBuckets(hist);
        maxBucketValueEver = std::max<uint32_t>(maxBucketValueEver, hist.collapsedBucketsMaxValue);
    }

    const auto yScale = -(height - 4) / (double)maxBucketValueEver;
    const auto yBase = height - 2;
    const auto yTop = 2;
    if (yTop < yBase)
    {
        for (auto& hist : m_histograms)
        {
			canvas::GeometryBuilder b(hist.geometry);
			b.blending(canvas::BlendOp::Addtive);
			b.beginPath();
			b.moveTo(0.0f, yBase);

			const auto numSamples = hist.collapsedBuckets.size();
			const auto xScale = width / (double)numSamples;

			for (uint32_t i = 0; i < numSamples; ++i)
			{
				const auto& data = hist.collapsedBuckets.typedData()[i];

				auto y = std::clamp<double>(yBase + data.bucketValue * yScale, yTop, yBase);
				auto x = i * xScale;
				b.lineTo(x, y);
			}
			b.lineTo(width, yBase);
			b.lineTo(0.0f, yBase);
			b.fillColor(hist.color);
			b.fill();
        }
    }
}

bool ImageHistogramWidget::handleMouseMovement(const input::MouseMovementEvent& evt)
{
    auto pos = (int)(evt.absolutePosition().x - cachedDrawArea().absolutePosition().x);
    if (pos != m_hoverPositionX)
    {
        m_hoverPositionX = pos;
        updateTooltip();
    }

    return TBaseClass::handleMouseMovement(evt);
}

void ImageHistogramWidget::handleHoverLeave(const ui::Position& absolutePosition)
{
    m_hoverPositionX = -1;
    return TBaseClass::handleHoverLeave(absolutePosition);
}

ui::ElementPtr ImageHistogramWidget::queryTooltipElement(const ui::Position& absolutePosition, ui::ElementArea& outArea) const
{
    if (m_histograms.empty())
        return nullptr;

    auto text = RefNew<ui::TextLabel>();
    m_activeTooltip = text.weak();

    outArea = cachedDrawArea();
    updateTooltip();

    return text;
}


void ImageHistogramWidget::updateTooltip() const
{
    if (auto tooltip = m_activeTooltip.lock())
    {
        if (m_hoverPositionX >= 0)
        {
            const double value = m_histogramMin + (m_histogramMax - m_histogramMin) * ((double)m_hoverPositionX / m_cachedHistogramGeometryRefSize.x);

            StringBuilder txt;
            txt.appendf("Bucket: {}   \n", Prec(value, 2));

            for (const auto& hist : m_histograms)
            {
                txt.appendf("{}: ", hist.caption);

                if (value >= hist.data->minValue && value <= hist.data->maxValue)
                {
                    const auto bucketSetp = hist.data->buckets.size() / (double)(hist.data->maxValue - hist.data->minValue);
                    const auto bucketIndex = (int)((value - hist.data->minValue) * bucketSetp);
                    if (bucketIndex >= 0 && bucketIndex <= hist.data->buckets.lastValidIndex())
                    {
                        const auto frac = hist.data->buckets[bucketIndex] / (double)m_histogramBucketMax;
                        txt.appendf("{}%", Prec(frac * 100.0, 1));

                        const auto maxFrac = hist.data->buckets[bucketIndex] / (double)hist.data->totalPixelCount;
                        txt.appendf(" ({}%)", Prec(maxFrac * 100.0, 1));
                    }
                }

                txt.append("      \n");
            }

            tooltip->text(txt.view());
                
        }
    }
}

void ImageHistogramWidget::renderForeground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);

    if (m_cachedHistogramGeometryRefSize != drawArea.size())
    {
        m_cachedHistogramGeometryRefSize = drawArea.size();
        cacheHistogramGeometry();
    }

    for (const auto& hist : m_histograms)
    {
        if (hist.geometry)
        {
			canvas.place(drawArea.absolutePosition(), hist.geometry);
        }
    }

    if (m_hoverPositionX >= 0 && !m_histograms.empty())
    {
		canvas::Geometry g;
		{
			canvas::GeometryBuilder b(g);
			b.beginPath();
			b.moveTo(m_hoverPositionX, 0);
			b.lineTo(m_hoverPositionX, drawArea.size().y);
			b.strokeColor(Color::WHITE);
			b.stroke();
		}

        canvas.place(drawArea.absolutePosition(), g);
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
