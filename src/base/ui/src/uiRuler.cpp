/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"
#include "uiRuler.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "uiStyleValue.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(HorizontalRuler);
        RTTI_METADATA(ElementClassNameMetadata).name("HorizontalRuler");
    RTTI_END_TYPE();

    HorizontalRuler::HorizontalRuler()
    {
        hitTest(true);
        allowFocusFromClick(true);
        enableAutoExpand(true, false);
    }

    void HorizontalRuler::region(float minVal, float maxVal)
    {
        m_viewRegionMin = minVal;
        m_viewRegionMax = maxVal;
    }

    void HorizontalRuler::activeRegion(float minVal, float maxVal)
    {
        m_viewActiveRegionMin = minVal;
        m_viewActiveRegionMax = maxVal;
    }

	void HorizontalRuler::generateGeometry(float width, float height) const
	{
		const auto minVal = m_viewRegionMin;
		const auto maxVal = m_viewRegionMax;

		const auto viewScale = width / (maxVal - minVal);
		const auto viewOffset = -minVal * viewScale;

		base::canvas::GeometryBuilder b(m_geometry);

		base::Color backgroundColor = base::Color::DARKGRAY;
		if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("background"_id))
			backgroundColor = shadowPtr->innerColor;

		const auto rulerTickColor = evalStyleValue<base::Color>("color"_id, base::Color::DARKGRAY);
		b.strokeColor(rulerTickColor);

		int labelStep = 10;
		while (labelStep * viewScale < 50.0f)
			labelStep *= 10;

		int tickStep = labelStep / 10;

		const auto minTickPos = (int)std::floor(minVal / tickStep);
		const auto maxTickPos = (int)std::ceil(maxVal / tickStep);
		for (int i = minTickPos; i <= maxTickPos; ++i)
		{
			int tickPos = i * tickStep;

			if (m_viewActiveRegionMin < m_viewActiveRegionMax)
			{
				if (tickPos < m_viewActiveRegionMin || tickPos > m_viewActiveRegionMax)
					continue;
			}

			const float x = tickPos * viewScale + viewOffset;

			float len = height * 0.25f;
			if (i % 10 == 0)
				len = height * 0.4f;

			b.beginPath();
			b.moveTo(x, height - len);
			b.lineTo(x, height);
			b.stroke();
		}

		if (m_viewActiveRegionMin < m_viewActiveRegionMax)
		{
			auto minX = m_viewActiveRegionMin * viewScale + viewOffset;
			if (minX > 0.0f)
			{
				b.fillColor(backgroundColor * 0.7f);
				b.beginPath();
				b.rect(0.0f, 0.0f, minX, height);
				b.fill();
			}

			auto maxX = m_viewActiveRegionMax * viewScale + viewOffset;
			if (maxX < width)
			{
				b.fillColor(backgroundColor * 0.7f);
				b.beginPath();
				b.rect(maxX, 0.0f, width - maxX, height);
				b.fill();
			}
		}

		if (const auto fonts = evalStyleValueIfPresentPtr<style::FontFamily>("font-family"_id))
		{
			//const auto bold = style::FontWeight::Bold == evalStyleValue("font-weight"_id, style::FontWeight::Normal);
			//const auto italic = style::FontStyle::Italic == evalStyleValue("font-style"_id, style::FontStyle::Normal);
			const auto size = std::max<uint32_t>(1, std::floorf(evalStyleValue<float>("font-size"_id, 14.0f) * cachedStyleParams().pixelScale));

			const auto minLabelPos = (int)std::floor(minVal / labelStep);
			const auto maxLabelPos = (int)std::ceil(maxVal / labelStep);
			for (int i = minLabelPos; i <= maxLabelPos; ++i)
			{
				int labelPos = i * labelStep;

				if (m_viewActiveRegionMin < m_viewActiveRegionMax)
				{
					if (labelPos < m_viewActiveRegionMin || labelPos > m_viewActiveRegionMax)
						continue;
				}

				const float x = labelPos * viewScale + viewOffset;

				b.pushTransform();
				b.translate(x, height * 0.6f);
				b.fillColor(rulerTickColor);
				b.print(fonts->normal, size, base::TempString("{}", labelPos), 0, 1);
				b.popTransform();
			}
		}
	}

    void HorizontalRuler::renderBackground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);

        if (m_viewRegionMin < m_viewRegionMax && drawArea.size().x > 0.0f)
        {
			generateGeometry(drawArea.size().x, drawArea.size().y);
            canvas.place(drawArea.absolutePosition(), m_geometry);
        }
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(VerticalRuler);
        RTTI_METADATA(ElementClassNameMetadata).name("VerticalRuler");
    RTTI_END_TYPE();

    VerticalRuler::VerticalRuler()
    {
        hitTest(true);
        allowFocusFromClick(true);
        enableAutoExpand(true, false);
    }

    void VerticalRuler::region(float minVal, float maxVal)
    {
        m_viewRegionMin = minVal;
        m_viewRegionMax = maxVal;
    }

    void VerticalRuler::activeRegion(float minVal, float maxVal)
    {
        m_viewActiveRegionMin = minVal;
        m_viewActiveRegionMax = maxVal;
    }

	void VerticalRuler::generateGeometry(float width, float height) const
	{
		const auto minVal = m_viewRegionMin;
		const auto maxVal = m_viewRegionMax;

		const auto viewScale = height / (maxVal - minVal);
		const auto viewOffset = -minVal * viewScale;

		base::canvas::GeometryBuilder b(m_geometry);

		base::Color backgroundColor = base::Color::DARKGRAY;
		if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("background"_id))
			backgroundColor = shadowPtr->innerColor;

		const auto rulerTickColor = evalStyleValue<base::Color>("color"_id, base::Color::DARKGRAY);
		b.strokeColor(rulerTickColor);

		int labelStep = 10;
		while (labelStep * viewScale < 50.0f)
			labelStep *= 10;

		int tickStep = labelStep / 10;

		const auto minTickPos = (int)std::floor(minVal / tickStep);
		const auto maxTickPos = (int)std::ceil(maxVal / tickStep);
		for (int i = minTickPos; i <= maxTickPos; ++i)
		{
			int tickPos = i * tickStep;

			if (m_viewActiveRegionMin < m_viewActiveRegionMax)
			{
				if (tickPos < m_viewActiveRegionMin || tickPos > m_viewActiveRegionMax)
					continue;
			}

			const float y = tickPos * viewScale + viewOffset;

			float len = width * 0.25f;
			if (i % 10 == 0)
				len = width * 0.4f;

			b.beginPath();
			b.moveTo(width - len, y);
			b.lineTo(width, y);
			b.stroke();
		}

		if (m_viewActiveRegionMin < m_viewActiveRegionMax)
		{
			auto minY = m_viewActiveRegionMin * viewScale + viewOffset;
			if (minY > 0.0f)
			{
				b.fillColor(backgroundColor * 0.7f);
				b.beginPath();
				b.rect(0.0f, 0.0f, width, minY);
				b.fill();
			}

			auto maxY = m_viewActiveRegionMax * viewScale + viewOffset;
			if (maxY < height)
			{
				b.fillColor(backgroundColor * 0.7f);
				b.beginPath();
				b.rect(0.0f, maxY, width, height - maxY);
				b.fill();
			}
		}

		if (const auto fonts = evalStyleValueIfPresentPtr<style::FontFamily>("font-family"_id))
		{
			//const auto bold = style::FontWeight::Bold == evalStyleValue("font-weight"_id, style::FontWeight::Normal);
			//const auto italic = style::FontStyle::Italic == evalStyleValue("font-style"_id, style::FontStyle::Normal);
			const auto size = std::max<uint32_t>(1, std::floorf(evalStyleValue<float>("font-size"_id, 14.0f) * cachedStyleParams().pixelScale));

			const auto minLabelPos = (int)std::floor(minVal / labelStep);
			const auto maxLabelPos = (int)std::ceil(maxVal / labelStep);
			for (int i = minLabelPos; i <= maxLabelPos; ++i)
			{
				int labelPos = i * labelStep;

				if (m_viewActiveRegionMin < m_viewActiveRegionMax)
				{
					if (labelPos < m_viewActiveRegionMin || labelPos > m_viewActiveRegionMax)
						continue;
				}

				const float y = labelPos * viewScale + viewOffset;

				b.pushTransform();
				b.translate(width * 0.9f, y + 2);
				b.fillColor(rulerTickColor);
				b.print(fonts->normal, size, base::TempString("{}", labelPos), 1, -1);
				b.popTransform();

			}
		}
	}

    void VerticalRuler::renderBackground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);

        if (m_viewRegionMin < m_viewRegionMax && drawArea.size().y > 0.0f)
        {
			generateGeometry(drawArea.size().x, drawArea.size().y);
			canvas.place(drawArea.absolutePosition(), m_geometry);
        }
    }

    //--

} // ui
