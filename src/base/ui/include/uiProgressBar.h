/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

/// a simple progress bar
class BASE_UI_API ProgressBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProgressBar, IElement);

public:
    ProgressBar(bool displayCaption=true);

    // get the current position (as a 0-1 fraction)
    INLINE float position() const { return m_pos; }

    //--

    // set the position fraction
    void position(float pos, base::StringView customText = "");

    //--

private:
    float m_pos;

    ElementPtr m_bar;
    TextLabelPtr m_text;

    //--

    virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
};

//--

END_BOOMER_NAMESPACE(ui)

