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

namespace ui
{

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

    ProgressBar::ProgressBar()
        : m_pos(0.5f)
    {
        layoutMode(LayoutMode::Vertical);

        m_bar = createChild<ProgressBarArea>();
        m_bar->ignoredInAutomaticLayout(true);
    }

    void ProgressBar::position(float pos)
    {
        auto clampedPos = std::clamp<float>(pos, 0.0f, 1.0f);
        if (m_pos != clampedPos)
        {
            m_pos = clampedPos;

            if (m_text)
                m_text->text(base::TempString("{}%", pos * 100.0f));

            invalidateLayout();
        }
    }

    bool ProgressBar::handleTemplateProperty(base::StringView name, base::StringView value)
    {
        if (name == "showPercents" || name == "text" || name == "showPercent")
        {
            bool flag = false;
            if (base::MatchResult::OK != value.match(flag))
                return false;

            if (flag && !m_text)
                m_text = createNamedChild<TextLabel>("ProgressCaption"_id, base::TempString("{}%", m_pos * 100.0f));

            return true;
        }
        else if (name == "value")
        {
            float pos = 0.0f;
            if (base::MatchResult::OK != value.match(pos))
                return false;
            position(pos);
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
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

} // ui

