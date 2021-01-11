/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiFourWaySplitter.h"
#include "uiInputAction.h"

#include "base/input/include/inputStructures.h"

namespace ui
{
    //--

    class FourWaySplitterSash : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FourWaySplitterSash, IElement);

    public:
        FourWaySplitterSash()
        {
            hitTest(HitTestState::DisabledTree);
            enableAutoExpand(true, true);
        }

        void dragging(bool flag)
        {
            if (flag != m_dragging)
            {
                m_dragging = flag;

                if (flag)
                    addStylePseudoClass("dragged"_id);
                else
                    removeStylePseudoClass("dragged"_id);
            }
        }

    private:
        bool m_dragging = false;
    };

    RTTI_BEGIN_TYPE_CLASS(FourWaySplitterSash);
        RTTI_METADATA(ElementClassNameMetadata).name("FourWaySplitterSash");
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(FourWaySplitter);
    RTTI_METADATA(ElementClassNameMetadata).name("FourWaySplitter");
    RTTI_END_TYPE();

    FourWaySplitter::FourWaySplitter(float splitX, float splitY)
        : m_splitFraction(splitX, splitY)
    {
        hitTest(true);
        enableAutoExpand(true, true);

        m_verticalSash = createInternalChild<FourWaySplitterSash>();
        m_verticalSash->addStyleClass("vertical"_id);

        m_horizontalSash = createInternalChild<FourWaySplitterSash>();
        m_horizontalSash->addStyleClass("horizontal"_id);
    }

    void FourWaySplitter::splitFraction(base::Vector2 frac)
    {
        base::Vector2 validFraction;
        validFraction.x = std::clamp<float>(frac.x, 0.0f, 1.0f);
        validFraction.y = std::clamp<float>(frac.y, 0.0f, 1.0f);

        if (m_splitFraction != validFraction)
        {
            m_splitFraction = validFraction;
            invalidateLayout();
        }
    }

    ElementArea FourWaySplitter::computeSashArea(const ElementArea& innerArea) const
    {
        auto sashSizeX = m_verticalSash->cachedLayoutParams().calcTotalSize().x;
        auto sashSizeY = m_horizontalSash->cachedLayoutParams().calcTotalSize().y;

        float sashPosX = std::max<float>(0.0f, (innerArea.size().x - sashSizeX) * m_splitFraction.x);
        float sashPosY = std::max<float>(0.0f, (innerArea.size().y - sashSizeY) * m_splitFraction.y);

        return ElementArea(innerArea.absolutePosition() + Position(sashPosX, sashPosY), Size(sashSizeX, sashSizeY));
    }

    void FourWaySplitter::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
    {
        const auto sashArea = computeSashArea(innerArea);

        ElementArea childAreas[4]; // TL TR BL BR
        childAreas[0] = ElementArea(innerArea.left(), innerArea.top(), sashArea.left(), sashArea.top());
        childAreas[1] = ElementArea(sashArea.right(), innerArea.top(), innerArea.right(), sashArea.top());
        childAreas[2] = ElementArea(innerArea.left(), sashArea.bottom(), sashArea.left(), innerArea.bottom());
        childAreas[3] = ElementArea(sashArea.right(), sashArea.bottom(), innerArea.right(), innerArea.bottom());

        uint32_t panelIndex = 0;
        for (ElementChildIterator it(childrenList()); it && (panelIndex < ARRAY_COUNT(childAreas)); ++it)
        {
            if (*it == m_verticalSash || *it == m_horizontalSash)
                continue;

            const auto& area = childAreas[panelIndex++];
            if (!area.empty())
                outArrangedChildren.add(*it, area, area.clipTo(clipArea));
        }

        {
            auto verticalArea = ElementArea(sashArea.left(), innerArea.top(), sashArea.right(), innerArea.bottom());
            outArrangedChildren.add(m_verticalSash, verticalArea, verticalArea.clipTo(clipArea));
        }

        {
            auto horizontalArea = ElementArea(innerArea.left(), sashArea.top(), innerArea.right(), sashArea.bottom());
            outArrangedChildren.add(m_horizontalSash, horizontalArea, horizontalArea.clipTo(clipArea));
        }
    }

    void FourWaySplitter::computeSize(Size& outSize) const
    {
        // nothing
    }

    void FourWaySplitter::inputActionFinished()
    {
        m_horizontalSash->dragging(false);
        m_verticalSash->dragging(false);
    }

    uint8_t FourWaySplitter::computeSashInputMask(const Position& pos) const
    {
        uint8_t mask = 0;

        if (m_verticalSash->cachedDrawArea().contains(pos))
            mask |= 1;
        if (m_horizontalSash->cachedDrawArea().contains(pos))
            mask |= 2;

        return mask;
    }

    InputActionPtr FourWaySplitter::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked())
        {
            auto mousePos = evt.absolutePosition().toVector();
            if (const auto inputMask = computeSashInputMask(mousePos))
            {
                const auto sashArea = computeSashArea(area);

                auto delta = mousePos - sashArea.absolutePosition();

                if (inputMask & 1)
                    m_verticalSash->dragging(true);
                if (inputMask & 2)
                    m_horizontalSash->dragging(true);

                return base::RefNew<FourWaySplitterSashInputAction>(this, area, delta, inputMask, sashArea.size());
            }
        }

        return InputActionPtr();
    }

    void FourWaySplitter::CursorForInputMask(uint8_t inputMask, base::input::CursorType& outCursorType)
    {
        if (inputMask == 3)
            outCursorType = base::input::CursorType::SizeAll;
        else if (inputMask == 1)
            outCursorType = base::input::CursorType::SizeWE;
        else if (inputMask == 2)
            outCursorType = base::input::CursorType::SizeNS;
    }

    bool FourWaySplitter::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        const auto mask = computeSashInputMask(absolutePosition);
        CursorForInputMask(mask, outCursorType);
        return true;
    }

    bool FourWaySplitter::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
    {
        // this is added so the splitter takes precedence over border
        outAreaType = base::input::AreaType::Client;
        return true;
    }

    //--

    class FourWaySplitterSashInputAction : public MouseInputAction
    {
    public:
        FourWaySplitterSashInputAction(FourWaySplitter* splitter, const ElementArea& sashArea, const Position& delta, uint8_t inputMask, Size sashSize)
            : MouseInputAction(splitter, base::input::KeyCode::KEY_MOUSE0)
            , m_splitter(splitter)
            , m_inputMask(inputMask)
            , m_sashSize(sashSize)
            , m_delta(delta)
        {
        }

        virtual ~FourWaySplitterSashInputAction()
        {
        }

        virtual void onCanceled() override
        {
            if (auto splitter = m_splitter.lock())
                splitter->inputActionFinished();
        }

        virtual void onFinished()  override
        {
            if (auto splitter = m_splitter.lock())
                splitter->inputActionFinished();
        }

        base::Vector2 computeTargetSashFraction(const Position& pos) const
        {
            if (auto splitter = m_splitter.lock())
            {
                auto sashPos = pos - m_delta;

                auto frac = splitter->splitFraction();

                if (m_inputMask & 1)
                {
                    auto pos = sashPos.x - splitter->cachedDrawArea().left();
                    auto area = (splitter->cachedDrawArea().size().x - m_sashSize.x);
                    frac.x = std::clamp<float>(pos / area, 0.0f, 1.0f);
                }

                if (m_inputMask & 2)
                {
                    auto pos = sashPos.y - splitter->cachedDrawArea().top();
                    auto area = (splitter->cachedDrawArea().size().y - m_sashSize.y);
                    frac.y = std::clamp<float>(pos / area, 0.0f, 1.0f);
                }

                return frac;
            }

            return base::Vector2(0.5f, 0.5f);
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (auto splitter = m_splitter.lock())
            {
                auto newSashFraction = computeTargetSashFraction(evt.absolutePosition().toVector());
                splitter->splitFraction(newSashFraction);
            }

            return InputActionResult();
        }

        virtual void onUpdateCursor(base::input::CursorType& outCursorType) override
        {
            FourWaySplitter::CursorForInputMask(m_inputMask, outCursorType);
        }

    private:
        base::RefWeakPtr<FourWaySplitter> m_splitter;
        Position m_delta;
        Size m_sashSize;
        uint8_t m_inputMask = 0;
    };

    //--    
 
} // ui