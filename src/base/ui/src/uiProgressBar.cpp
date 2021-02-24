/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"

#include "uiProgressBar.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE(ui)

//---

// active area element, does nothing but is
class ProgressBarArea : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProgressBarArea, IElement);

public:
    ProgressBarArea()
    {}
};

RTTI_BEGIN_TYPE_CLASS(ProgressBarArea);
    RTTI_METADATA(ElementClassNameMetadata).name("ProgressBarArea");
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ProgressBar);
    RTTI_METADATA(ElementClassNameMetadata).name("ProgressBar");
RTTI_END_TYPE();

ProgressBar::ProgressBar(bool displayCaption /*= true*/)
    : m_pos(0.5f)
{
    layoutMode(LayoutMode::Vertical);

    m_bar = createChild<ProgressBarArea>();
    m_bar->ignoredInAutomaticLayout(true);

    if (displayCaption)
        m_text = createNamedChild<TextLabel>("ProgressCaption"_id, base::TempString("{}%", m_pos * 100.0f));
}

void ProgressBar::position(float pos, base::StringView customText)
{
    auto clampedPos = std::clamp<float>(pos, 0.0f, 1.0f);
    if (m_pos != clampedPos)
    {
        m_pos = clampedPos;

        if (m_text)
        {
            if (customText.empty())
                m_text->text(base::TempString("{}%", Prec(pos * 100.0f, 1)));
            else
                m_text->text(base::TempString("{}%: {}", Prec(pos * 100.0f, 1), customText));
        }

        invalidateLayout();
    }
}

void ProgressBar::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
{
    if (m_pos > 0.0f)
    {
        auto pos = innerArea.left() + (float)(innerArea.size().x * m_pos);
        auto area = ElementArea(innerArea.left(), innerArea.top(), pos, innerArea.bottom());

        outArrangedChildren.add(m_bar, area, clipArea);
    }

    return TBaseClass::arrangeChildren(innerArea, clipArea, outArrangedChildren, dynamicSizing);
}

//---

END_BOOMER_NAMESPACE(ui)
