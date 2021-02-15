/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElement.h"
#include "uiStyleSelector.h"
#include "uiStyleLibrary.h"
#include "uiElementStyle.h"
#include "uiStyleValue.h"
#include "uiScrollBar.h"
#include "uiElementHitCache.h"
#include "uiListView.h"

namespace ui
{
    //--

    void ArrangeAxis(float requiredSize, float allowedSize, ElementVerticalLayout mode, float& outSize, float& outOffset)
    {
        // expand mode works a little bit differently
        if (mode == ElementVerticalLayout::Expand)
        {
            outSize = allowedSize;
            outOffset = 0.0f;
        }
        else
        {
            // if the container is smaller than what we want to be than shrink as well
            float retSize = std::min(requiredSize, allowedSize);

            // align in the allowed space
            if (mode == ElementVerticalLayout::Top)
            {
                outSize = retSize;
                outOffset = 0.0f;
            }
            else if (mode == ElementVerticalLayout::Middle)
            {
                outSize = retSize;
                outOffset = (allowedSize - retSize) / 2.0f;
            }
            else if (mode == ElementVerticalLayout::Bottom)
            {
                outSize = retSize;
                outOffset = (allowedSize - retSize);
            }
            else
            {
                ASSERT(!"Invalid alignment mode");
            }
        }
    }

    void ArrangeAxis(float requiredSize, float allowedSize, ElementHorizontalLayout mode, float& outSize, float& outOffset)
    {
        return ArrangeAxis(requiredSize, allowedSize, (ElementVerticalLayout)mode, outSize, outOffset);
    }

    ElementArea ArrangeElementLayout(const ElementArea& incomingInnerArea, const ElementLayout& layout, const Size& totalSize)
    {
        // limit size
        Size size(0, 0);
        Position offset(0,0);
        ArrangeAxis(totalSize.x, incomingInnerArea.size().x, layout.m_horizontalAlignment, size.x, offset.x);
        ArrangeAxis(totalSize.y, incomingInnerArea.size().y, layout.m_verticalAlignment, size.y, offset.y);

        // we have the relative placement and size of the element inside the provided area
        // the placement is for the WHOLE element (with margins and border)
        auto absoluteOffset = incomingInnerArea.absolutePosition() + offset;
        absoluteOffset.x = std::round(absoluteOffset.x);
        absoluteOffset.y = std::round(absoluteOffset.y);
        return ElementArea(absoluteOffset, size);
    }

    ElementArea ArrangeAreaLayout(const ElementArea& incomingInnerArea, const Size& totalSize, ElementVerticalLayout verticalAlign, ElementHorizontalLayout horizontalAlign)
    {
        // limit size
        Size size(0, 0);
        Position offset(0, 0);
        ArrangeAxis(totalSize.x, incomingInnerArea.size().x, verticalAlign, size.x, offset.x);
        ArrangeAxis(totalSize.y, incomingInnerArea.size().y, horizontalAlign, size.y, offset.y);

        // we have the relative placement and size of the element inside the provided area
        // the placement is for the WHOLE element (with margins and border)
        auto absoluteOffset = incomingInnerArea.absolutePosition() + offset;
        absoluteOffset.x = (float)(int)absoluteOffset.x;
        absoluteOffset.y = (float)(int)absoluteOffset.y;
        return ElementArea(absoluteOffset, size);
    }

    //---

    static bool ConsiderElem(const IElement* element)
    {
        return (element->visibility() == VisibilityState::Visible) && !element->isIgnoredInAutomaticLayout() && !element->isOverlay();
    }

	bool IElement::iterateDrawChildren(ElementDrawListToken& token) const
	{
		if (nullptr == token.curToken)
		{
			if (nullptr == m_firstChild)
				return false;

			token.curToken = nullptr;
			token.nextToken = m_firstChild;
		}

        while (token.nextToken != nullptr)
        {
            token.curToken = token.nextToken;
            token.elem = (IElement*)token.curToken;
            token.nextToken = ((IElement*)token.nextToken)->m_listNext;

			ASSERT(token.nextToken != token.curToken);
            if (ConsiderElem(token.elem))
                return true;
        }

        return false;
    }

    //---

    void IElement::arrangeChildrenColumns(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const
    {
        float xPos = innerArea.left();

        // no columns pack as horizontal sizer
        if (nullptr == dynamicSizing)
        {
            const ElementDynamicSizing* noDynamicSizing = nullptr;
            arrangeChildrenHorizontal(innerArea, clipArea, outArrangedChildren, noDynamicSizing, outActualSizeUsed);
            return;
        }

        // collect elements
        base::InplaceArray<const IElement*, 10> columnsContent;
        {
            ElementDrawListToken it;
            while (iterateDrawChildren(it))
                columnsContent.pushBack(*it);
        }

        // add elements from columns
        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
        if (auto numColumns = dynamicSizing->m_numColumns)
        {
            float right = dynamicSizing->m_columns[numColumns - 1].m_right;
            int columnIndex = numColumns - 1;
            while (columnIndex >= 0)
            {
                if (columnIndex <= columnsContent.lastValidIndex())
                {
                    auto left = std::max<float>(xPos, dynamicSizing->m_columns[columnIndex].m_left);
                    auto columnArea = ElementArea(left, innerArea.top(), right, innerArea.bottom());

                    if (auto content = columnsContent[columnIndex])
                    {
                        if (!columnArea.empty())
                        {
                            auto elemSize = content->cachedLayoutParams().calcTotalSize();

                            if (dynamicSizing->m_columns[columnIndex].m_center)
                                const_cast<ElementLayout&>(content->cachedLayoutParams()).m_horizontalAlignment = ElementHorizontalLayout::Center;

                            auto arangedArea = ArrangeElementLayout(columnArea, content->cachedLayoutParams(), elemSize);
                            auto localClipArea = ElementArea(std::max(left, clipArea.left()), clipArea.top(), std::min(right, clipArea.right()), clipArea.bottom());
                            outArrangedChildren.add(content, arangedArea, localClipArea);

                            totalHeight = std::max(totalHeight, elemSize.y);
                        }
                    }

                    totalWidth += (right - left);
                    right = left;
                }

                columnIndex -= 1;
            }
        }

        // report actual size used
        outActualSizeUsed.x = std::max(outActualSizeUsed.x, totalWidth);
        outActualSizeUsed.y = std::max(outActualSizeUsed.y, totalHeight);
    }

    void IElement::arrangeChildrenHorizontal(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const
    {
        // TODO: cache !
        bool hasExpandableVerticalElements = false;
        float totalProportion = 0.0f;
        float totalSizeForNormalElements = 0.0f;
        {
            ElementDrawListToken it;
            while (iterateDrawChildren(it))
            {
                const auto &elementLayout = it->cachedLayoutParams();
                if (elementLayout.m_proportion > 0.0f)
                    totalProportion += elementLayout.m_proportion;
                else if (elementLayout.m_horizontalAlignment == ElementHorizontalLayout::Expand)
                    totalProportion += 1.0f;
                else
                    totalSizeForNormalElements += elementLayout.calcTotalSize().x;

                if (elementLayout.m_verticalAlignment == ElementVerticalLayout::Expand)
                    hasExpandableVerticalElements = true;
            }
        }

        auto leftOverSize = std::max<float>(0.0f, innerArea.size().x - totalSizeForNormalElements);
        auto maxExpandSize = leftOverSize / std::max<float>(1.0f, totalProportion); // allows elements to "allocate" 20% of space etc
        auto totalUnallocatedSpace = innerArea.size().x - totalSizeForNormalElements - (totalProportion * maxExpandSize); // left over space we can use for internal alignment

        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
        float pos = innerArea.absolutePosition().x;

        if (cachedLayoutParams().m_internalHorizontalAlignment == ElementHorizontalLayout::Center)
            pos += totalUnallocatedSpace * 0.5f;
        else if (cachedLayoutParams().m_internalHorizontalAlignment == ElementHorizontalLayout::Right)
            pos += totalUnallocatedSpace;

        {
            ElementDrawListToken it;
            while (iterateDrawChildren(it))
            {
                const auto &elementLayout = it->cachedLayoutParams();
                auto elementLayoutSize = elementLayout.calcTotalSize();

                ElementArea sliceArea;
                if (elementLayout.m_proportion > 0.0f)
                    sliceArea = innerArea.horizontalSice(pos, pos + maxExpandSize * elementLayout.m_proportion);
                else if (elementLayout.m_horizontalAlignment == ElementHorizontalLayout::Expand)
                    sliceArea = innerArea.horizontalSice(pos, pos + maxExpandSize);
                else
                    sliceArea = innerArea.horizontalSice(pos, pos + elementLayoutSize.x);

                auto area = ArrangeElementLayout(sliceArea, elementLayout, elementLayoutSize);
                outArrangedChildren.add(*it, area, clipArea);

                pos += sliceArea.size().x;
                totalWidth += area.size().x;
                totalHeight = std::max(totalHeight, elementLayoutSize.y);

                if (nullptr != it.m_unadjustedArea)
                    * it.m_unadjustedArea = area;
            }
        }

        // report actual size used
        outActualSizeUsed.x = std::max(outActualSizeUsed.x, totalWidth);
        outActualSizeUsed.y = std::max(outActualSizeUsed.y, totalHeight);
    }

    void IElement::arrangeChildrenVertical(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const
    {
        float totalProportion = 0.0f;
        float totalSizeForNormalElements = 0.0f;
        {
            ElementDrawListToken it;
            while (iterateDrawChildren(it))
            {
                const auto& elementLayout = it->cachedLayoutParams();
                if (elementLayout.m_proportion > 0.0f)
                    totalProportion += elementLayout.m_proportion;
                else if (elementLayout.m_verticalAlignment == ElementVerticalLayout::Expand)
                    totalProportion += 1.0f;
                else
                    totalSizeForNormalElements += elementLayout.calcTotalSize().y;
            }
        }

        totalProportion = std::max(1.0f, totalProportion);

        auto leftOverSize = std::max<float>(0.0f, innerArea.size().y - totalSizeForNormalElements);
        auto maxExpandSize = leftOverSize / totalProportion;
        auto totalUnallocatedSpace = std::max<float>(0.0f, innerArea.size().y - totalSizeForNormalElements - (totalProportion * maxExpandSize)); // left over space we can use for internal alignment

        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
        float pos = innerArea.absolutePosition().y;
        {
            ElementDrawListToken it;
            while (iterateDrawChildren(it))
            {
                const auto& elementLayout = it->cachedLayoutParams();
                auto elementLayoutSize = elementLayout.calcTotalSize();

                ElementArea sliceArea;
                if (elementLayout.m_proportion > 0.0f)
                    sliceArea = innerArea.verticalSlice(pos, pos + maxExpandSize * elementLayout.m_proportion);
                else if (elementLayout.m_verticalAlignment == ElementVerticalLayout::Expand)
                    sliceArea = innerArea.verticalSlice(pos, pos + maxExpandSize);
                else
                    sliceArea = innerArea.verticalSlice(pos, pos + elementLayoutSize.y);

                /*if ((m_scrollBarState & 1) && (elementLayoutSize.x > sliceArea.size().x))
                    sliceArea = sliceArea.resize(Size(elementLayoutSize.x, sliceArea.size().y));*/

                auto area = ArrangeElementLayout(sliceArea, it->cachedLayoutParams(), elementLayoutSize);
                outArrangedChildren.add(*it, area, clipArea);

                pos += sliceArea.size().y;
                totalWidth = std::max(totalWidth, elementLayoutSize.x);
                totalHeight += area.size().y;

                if (nullptr != it.m_unadjustedArea)
                    * it.m_unadjustedArea = area;
            }
        }

        // report actual size used
        outActualSizeUsed.x = std::max(outActualSizeUsed.x, totalWidth);
        outActualSizeUsed.y = std::max(outActualSizeUsed.y, totalHeight);
    }

    void IElement::arrangeChildrenOverlay(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, Size& outActualSizeUsed) const
    {
        float totalWidth = 0.0f;
        float totalHeight = 0.0f;

        if (m_overlayElements)
        {
            for (auto* it : *m_overlayElements)
            {
                if (it->visibility() == VisibilityState::Visible)
                {
                    const auto& elementLayout = it->cachedLayoutParams();
                    auto elementLayoutSize = elementLayout.calcTotalSize();

                    totalWidth = std::max(totalWidth, elementLayoutSize.x);
                    totalHeight = std::max(totalHeight, elementLayoutSize.y);

                    auto area = ArrangeElementLayout(innerArea, elementLayout, elementLayoutSize);
                    outArrangedChildren.add(it, area, clipArea);
                }
            }
        }

        // report actual size used
        outActualSizeUsed.x = std::max(outActualSizeUsed.x, totalWidth);
        outActualSizeUsed.y = std::max(outActualSizeUsed.y, totalHeight);
    }

    void IElement::arrangeChildrenIcons(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const
    {
        float rowWidth = innerArea.size().x;

        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
        float rowHeight = 0.0f;
        float rowPos = 0.0f;
        float yPos = 0.0f;

        ElementDrawListToken it;
        while (iterateDrawChildren(it))
        {
            const auto& elementLayout = it->cachedLayoutParams();
            auto elementLayoutSize = elementLayout.calcTotalSize();

            if (rowPos && rowPos + elementLayoutSize.x > rowWidth)
            {
                yPos += rowHeight;
                rowPos = 0.0f;
                rowHeight = 0.0f;
            }

            auto elementArea = innerArea.offset(Position(rowPos, yPos)).resize(elementLayoutSize);
            auto area = ArrangeElementLayout(elementArea, elementLayout, elementLayoutSize);
            outArrangedChildren.add(*it, area, clipArea);

            rowPos += elementLayoutSize.x;
            totalWidth = std::max(totalWidth, rowPos);
            totalHeight = std::max(totalHeight, yPos + elementLayoutSize.y);
            rowHeight = std::max(rowHeight, elementLayoutSize.y);

            if (rowPos > rowWidth)
            {
                yPos += rowHeight;
                rowPos = 0.0f;
                rowHeight = 0.0f;
            }

            if (nullptr != it.m_unadjustedArea)
                * it.m_unadjustedArea = area;
        }

        // report actual size used
        outActualSizeUsed.x = std::max(outActualSizeUsed.x, totalWidth);
        outActualSizeUsed.y = std::max(outActualSizeUsed.y, totalHeight);
    }

    void IElement::arrangeChildrenInner(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed, uint8_t& outNeededScrollbar) const
    {
        // do arrangement
        uint8_t possibleScrollBarMask = 0;
        switch (m_layout)
        {
            case LayoutMode::Horizontal:
                arrangeChildrenHorizontal(innerArea, clipArea, outArrangedChildren, dynamicSizing, outActualSizeUsed);
                possibleScrollBarMask = 3; // both
                break;

            case LayoutMode::Vertical:
                arrangeChildrenVertical(innerArea, clipArea, outArrangedChildren, dynamicSizing, outActualSizeUsed);
                possibleScrollBarMask = 3; // both
                break;

            case LayoutMode::Columns:
                arrangeChildrenColumns(innerArea, clipArea, outArrangedChildren, dynamicSizing, outActualSizeUsed);
                possibleScrollBarMask = 0; // none
                break;

            case LayoutMode::Icons:
                arrangeChildrenIcons(innerArea, clipArea, outArrangedChildren, dynamicSizing, outActualSizeUsed);
                possibleScrollBarMask = 2; // vertical only
                break;
        }

        // arrange overlay items
        if (m_renderOverlayElements)
            arrangeChildrenOverlay(innerArea, clipArea, outArrangedChildren, outActualSizeUsed);

        // check horizontal scroll bar requirements
        outNeededScrollbar = 0;
        if (possibleScrollBarMask & 1)
        {
            if (outActualSizeUsed.x > innerArea.size().x) // we use more than we have
                outNeededScrollbar |= 1;
        }
        if (possibleScrollBarMask & 2)
        {
            if (outActualSizeUsed.y > innerArea.size().y) // we use more than we have
                outNeededScrollbar |= 2;
        }
    }

    void IElement::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
    {
        Size totalUsedSize;
        uint8_t neededScrollBars = 0;
        arrangeChildrenInner(innerArea, clipArea, outArrangedChildren, dynamicSizing, totalUsedSize, neededScrollBars);
    }

    //--

    void IElement::computeSizeOverlay(Size& outSize) const
    {
        if (m_overlayElements)
        {
            for (auto* element : *m_overlayElements)
            {
                if (element->visibility() == VisibilityState::Visible)
                {
                    auto elementLayoutSize = element->cachedLayoutParams().calcTotalSize();
                    outSize.x = std::max(outSize.x, elementLayoutSize.x);
                    outSize.y = std::max(outSize.y, elementLayoutSize.y);
                }
            }
        }
    }

    void IElement::computeSizeVertical(Size& outSize) const
    {
        ElementDrawListToken it;
        while (iterateDrawChildren(it))
        {
            auto elementLayoutSize = it->cachedLayoutParams().calcTotalSize();
            outSize.x = std::max(outSize.x, elementLayoutSize.x);
            outSize.y += elementLayoutSize.y;
        }
    }

    void IElement::computeSizeHorizontal(Size& outSize) const
    {
        ElementDrawListToken it;
        while (iterateDrawChildren(it))
        {
            auto elementLayoutSize = it->cachedLayoutParams().calcTotalSize();
            outSize.x += elementLayoutSize.x;
            outSize.y = std::max(outSize.y, elementLayoutSize.y);
        }
    }

    void IElement::computeSizeIcons(ui::Size &outSize) const
    {
        // there's no way to compute the size since it depends on the target, we fill the list automatically
        outSize = Size(10, 10);
    }

    void IElement::computeSize(Size& outSize) const
    {
        switch (m_layout)
        {
            case LayoutMode::Horizontal:
                computeSizeHorizontal(outSize);
                break;

            case LayoutMode::Vertical:
                computeSizeVertical(outSize);
                break;

            case LayoutMode::Columns:
                computeSizeHorizontal(outSize);
                outSize.x = 1.0f; // not known
                break;

            case LayoutMode::Icons:
                computeSizeIcons(outSize);
                break;
        }

        //computeSizeOverlay(outSize);
    }

    //---

} // ui