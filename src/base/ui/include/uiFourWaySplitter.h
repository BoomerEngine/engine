/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

class FourWaySplitterSash;

/// four way splitter
class BASE_UI_API FourWaySplitter : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(FourWaySplitter, IElement);

public:
    FourWaySplitter(float splitX=0.5f, float splitY=0.5f);
        
    /// get splitter fraction (0-1)
    INLINE base::Vector2 splitFraction() const { return m_splitFraction; }

    //--

    // set splitter position
    void splitFraction(base::Vector2 frac);

    //--

private:
    base::RefPtr<FourWaySplitterSash> m_verticalSash;
    base::RefPtr<FourWaySplitterSash> m_horizontalSash;

    base::Vector2 m_splitFraction;

    //--

    virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
    virtual void computeSize(Size& outSize) const override;

    //--

    ElementArea computeSashArea(const ElementArea& area) const;
    uint8_t computeSashInputMask(const Position& pos) const;

    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
    virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const override final;

    void inputActionFinished();

    static void CursorForInputMask(uint8_t mask, base::input::CursorType& outCursorType);

    friend class FourWaySplitterSashInputAction;    
};
   
//--

END_BOOMER_NAMESPACE(ui)