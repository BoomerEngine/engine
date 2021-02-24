/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiSplitter.h"
#include "uiInputAction.h"

#include "base/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_CLASS(Splitter);
RTTI_METADATA(ElementClassNameMetadata).name("Splitter");
RTTI_END_TYPE();

Splitter::Splitter()
{
    enableAutoExpand(true, true);
}

Splitter::Splitter(Direction splitDirection, float initialSplitFraction, char splitSide)
    : m_splitDirection(splitDirection)
    , m_splitFraction(initialSplitFraction)
    , m_splitSide(splitSide)
    , m_splitConstraint(0.0f)
{
    addStyleClass(m_splitDirection == Direction::Vertical ? "vertical"_id : "horizontal"_id);
    addStyleClass("expand"_id);
    enableAutoExpand(true, true);

    m_sash = createInternalChild<SplitterSash>(this);
}

void Splitter::splitConstraint(float constraint)
{
    if (m_splitConstraint != constraint)
    {
        m_splitConstraint = constraint;
        invalidateLayout();
    }
}

void Splitter::splitFraction(float fraction)
{
    auto validFraction = std::clamp<float>(fraction, 0.0f, 1.0f);
    if (m_splitFraction != validFraction)
    {
        m_splitFraction = validFraction;
        invalidateLayout();
    }
}

static float LimitSashPosition(float position, float maxPosition, float limit)
{
    if (limit > 0.0f)
        return std::clamp(position, limit, maxPosition);
    else if (limit < 0.0f)
        return std::clamp(position, 0.0f, maxPosition + limit);
    else
        return position;
}

bool Splitter::handleTemplateProperty(base::StringView name, base::StringView value)
{
    if (name == "fraction")
    {
        float val = 0.5f;
        if (base::MatchResult::OK != value.match(val))
            return false;

        splitFraction(val);
        return true;
    }
    else if (name == "constraint")
    {
        float val = 0.5f;
        if (base::MatchResult::OK != value.match(val))
            return false;

        splitConstraint(val);
        return true;
    }
    else if (name == "direction")
    {
        if (value == "vertical")
        {
            m_splitDirection = Direction::Vertical;
            removeStyleClass("horizontal"_id);
            addStyleClass("vertical"_id);
            return true;
        }
        else if (value == "horizontal")
        {
            m_splitDirection = Direction::Horizontal;
            addStyleClass("horizontal"_id);
            removeStyleClass("vertical"_id);
            return true;
        }
        else
        {
            return false;
        }
    }

    return TBaseClass::handleTemplateProperty(name, value);
}

void Splitter::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
{
    IElement* first = nullptr, * second = nullptr;
    collectChildren(first, second);

    if (first && second)
    {
        auto firstSize = first->cachedLayoutParams().calcTotalSize();
        auto sashSize = m_sash->cachedLayoutParams().calcTotalSize();
        auto secondSize = second->cachedLayoutParams().calcTotalSize();

        if (m_splitDirection == Direction::Vertical)
        {
            auto maxSashPosition = innerArea.size().x - sashSize.x;
            auto physicalSashPosition = LimitSashPosition(maxSashPosition * m_splitFraction, maxSashPosition, m_splitConstraint);

            auto sashPosition = innerArea.left() + physicalSashPosition;

            {
                auto firstArea = innerArea.horizontalSice(innerArea.left(), sashPosition);
                if (!firstArea.empty())
                    outArrangedChildren.add(first, firstArea, firstArea.clipTo(clipArea));
            }

            {
                auto sashArea = innerArea.horizontalSice(sashPosition, sashPosition + sashSize.x);
                outArrangedChildren.add(m_sash, sashArea, clipArea);
            }

            {
                auto secondArea = innerArea.horizontalSice(sashPosition + sashSize.x, innerArea.right());
                if (!secondArea.empty())
                    outArrangedChildren.add(second, secondArea, secondArea.clipTo(clipArea));
            }
        }
        else if (m_splitDirection == Direction::Horizontal)
        {
            auto maxSashPosition = innerArea.size().y - sashSize.y;
            auto physicalSashPosition = LimitSashPosition(maxSashPosition * m_splitFraction, maxSashPosition, m_splitConstraint);

            auto sashPosition = innerArea.top() + physicalSashPosition;

            {
                auto firstArea = innerArea.verticalSlice(innerArea.top(), sashPosition);
                if (!firstArea.empty())
                    outArrangedChildren.add(first, firstArea, firstArea.clipTo(clipArea));
            }

            {
                auto sashArea = innerArea.verticalSlice(sashPosition, sashPosition + sashSize.y);
                outArrangedChildren.add(m_sash, sashArea, clipArea);
            }

            {
                auto secondArea = innerArea.verticalSlice(sashPosition + sashSize.y, innerArea.bottom());
                if (!secondArea.empty())
                    outArrangedChildren.add(second, secondArea, secondArea.clipTo(clipArea));
            }
        }
    }
    else if (first)
    {
        outArrangedChildren.add(first, innerArea, clipArea);
    }
}

void Splitter::collectChildren(IElement*& outFirst, IElement*& outSecond) const
{
    for (ElementChildIterator it(childrenList()); it; ++it)
    {
        if (*it == m_sash)
            continue;

        if (outFirst == nullptr)
            outFirst = *it;
        else if (outSecond == nullptr)
            outSecond = *it;
    }
}

void Splitter::computeSize(Size& outSize) const
{
    IElement* first = nullptr, *second = nullptr;
    collectChildren(first, second);

    if (first && second)
    {
        auto firstSize = first->cachedLayoutParams().calcTotalSize();
        auto secondSize = second->cachedLayoutParams().calcTotalSize();
        auto sashSize = m_sash->cachedLayoutParams().calcTotalSize();
        if (m_splitDirection == Direction::Vertical)
        {
            outSize.x = firstSize.x + sashSize.x + secondSize.x;
            outSize.y = std::max(firstSize.y, secondSize.y);
        }
        else
        {
            outSize.x = std::max(firstSize.x, secondSize.x);
            outSize.y = firstSize.y + sashSize.y + secondSize.y;
        }
    }
    else if (first)
    {
        auto firstSize = first->cachedLayoutParams().calcTotalSize();
        outSize = firstSize;
    }
}

//--

class SpliterSashInputAction : public MouseInputAction
{
public:
    SpliterSashInputAction(SplitterSash* sash, const ElementArea& sashArea, const Position& delta)
        : MouseInputAction(sash, base::input::KeyCode::KEY_MOUSE0)
        , m_sash(sash)
        , m_delta(delta)
    {
    }

    virtual ~SpliterSashInputAction()
    {
    }

    virtual void onCanceled() override
    {
        if (auto sash = m_sash.lock())
            sash->inputActionFinished();
    }

    virtual void onFinished()  override
    {
        if (auto sash = m_sash.lock())
            sash->inputActionFinished();
    }

    float computeTargetSashFraction(const Position& pos) const
    {
        if (auto sash = m_sash.lock())
        {
            auto sashPos = pos - m_delta;
            auto sashSize = sash->cachedLayoutParams().calcTotalSize();

            auto* splitter = sash->parentSplitter();
            if (splitter->splitDirection() == Direction::Vertical)
            {
                auto pos = sashPos.x - splitter->cachedDrawArea().left();
                auto area = splitter->cachedDrawArea().size().x - sashSize.x;
                return std::clamp<float>(pos / area, 0.0f, 1.0f);
            }
            else
            {
                auto pos = sashPos.y - splitter->cachedDrawArea().top();
                auto area = splitter->cachedDrawArea().size().y - sashSize.y;
                return std::clamp<float>(pos / area, 0.0f, 1.0f);
            }
        }

        return 0.0f;
    }

    virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        if (auto sash = m_sash.lock())
        {
            auto newSashFraction = computeTargetSashFraction(evt.absolutePosition().toVector());
            sash->parentSplitter()->splitFraction(newSashFraction);
        }

        return InputActionResult();
    }

    virtual void onUpdateCursor(base::input::CursorType& outCursorType) override
    {
        if (auto sash = m_sash.lock())
        {
            if (sash->parentSplitter()->splitDirection() == Direction::Vertical)
                outCursorType = base::input::CursorType::SizeWE;
            else
                outCursorType = base::input::CursorType::SizeNS;
        }
    }

private:
    base::RefWeakPtr<SplitterSash> m_sash;
    Position m_delta;
};

//--

RTTI_BEGIN_TYPE_CLASS(SplitterSash);
RTTI_METADATA(ElementClassNameMetadata).name("SplitterSash");
RTTI_END_TYPE();

SplitterSash::SplitterSash(Splitter* splitter /*= nullptr*/)
    : m_splitter(splitter)
    , m_dragging(0)
{
    hitTest(HitTestState::Enabled);
    enableAutoExpand(true, true);
}

InputActionPtr SplitterSash::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
{
    if (evt.leftClicked())
    {
        dragging(true);

        auto mousePos = evt.absolutePosition().toVector();
        auto delta = mousePos - cachedDrawArea().absolutePosition();
        return base::RefNew<SpliterSashInputAction>(this, area, delta);
    }

    return InputActionPtr();
}

void SplitterSash::dragging(bool flag)
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

void SplitterSash::inputActionFinished()
{
    dragging(false);
    invalidateStyle();
}

bool SplitterSash::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
{
    outCursorType = (m_splitter->splitDirection() == Direction::Vertical) ? base::input::CursorType::SizeWE : base::input::CursorType::SizeNS;
    return true;
}

bool SplitterSash::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
{
    // this is added so the splitter takes precedence over border
    outAreaType = base::input::AreaType::Client;
    return true;
}

END_BOOMER_NAMESPACE(ui)