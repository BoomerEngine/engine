/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{

    namespace prv
    {
        struct LayoutData;
        struct LayoutDisplayData;
    } // prv

    //---

    /// a simple non-editable text label
    class BASE_UI_API TextLabel : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextLabel, IElement);

    public:
        TextLabel();
        TextLabel(base::StringView<char> txt);
        virtual ~TextLabel();

        // get current text
        INLINE const base::StringBuf& text() const { return m_text; }

        //--

        // set text
        void text(base::StringView<char> txt);

        // set the highlight range
        void highlight(int startPos, int endPos);

        // reset the highlight
        void resetHighlight();

    protected:
        virtual void computeLayout(ElementLayout& outLayout) override;
        virtual void computeSize(Size& outSize) const override;
        virtual void prepareShadowGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void prepareForegroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override;

        //--

        base::StringBuf m_text;

        prv::LayoutData* m_data = nullptr;
        prv::LayoutDisplayData* m_displayData = nullptr;
    };

    //---

} // ui