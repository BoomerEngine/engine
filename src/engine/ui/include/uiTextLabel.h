/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

namespace prv
{
    struct LayoutData;
    struct LayoutDisplayData;
} // prv

//---

/// a simple non-editable text label
class ENGINE_UI_API TextLabel : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(TextLabel, IElement);

public:
    TextLabel();
    TextLabel(StringView txt);
    virtual ~TextLabel();

    // get current text
    INLINE const StringBuf& text() const { return m_text; }

    //--

    // set text
    void text(StringView txt);

    // set the highlight range
    void highlight(int startPos, int endPos);

    // reset the highlight
    void resetHighlight();

protected:
    virtual void computeLayout(ElementLayout& outLayout) override;
    virtual void computeSize(Size& outSize) const override;
    virtual void prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    virtual void prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    
    //--

    StringBuf m_text;

    prv::LayoutData* m_data = nullptr;
    prv::LayoutDisplayData* m_displayData = nullptr;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
