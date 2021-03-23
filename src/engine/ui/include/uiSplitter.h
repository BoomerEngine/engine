/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class SplitterSash;

/// binary splitter
class ENGINE_UI_API Splitter : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(Splitter, IElement);

public:
    Splitter();
    Splitter(Direction splitDirection, float initialSplitFraction = 0.5f, char splitSide = 0);

    /// get splitter fraction (0-1)
    INLINE float splitFraction() const { return m_splitFraction; }

    /// get slit side (0 or 1, determines which side is Left/Top or Right/Bottom)
    INLINE int splitSide() const { return m_splitSide;}

    /// get splitter direction
    INLINE Direction splitDirection() const { return m_splitDirection; }

    //--

    // set splitter position
    // NOTE: splitter position is fractional value between 0 and 1
    void splitFraction(float position);

    // set min size, if positive it's for the first target if negative it's for the second target
    void splitConstraint(float constraint);

    //--

private:
    RefPtr<SplitterSash> m_sash;

    float m_splitFraction;
    Direction m_splitDirection;
    float m_splitConstraint;
    char m_splitSide;

    //--

    virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
    virtual void computeSize(Size& outSize) const override;

    void collectChildren(IElement*& outFirst, IElement*& outSecond) const;
};

//--

/// splitter sash that can be dragged
class ENGINE_UI_API SplitterSash : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(SplitterSash, IElement);

public:
    SplitterSash(Splitter* splitter = nullptr);

    /// get the splitter that owns this sash
    INLINE Splitter* parentSplitter() const { return m_splitter; }

private:
    // IElement
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, CursorType& outCursorType) const override;
    virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, AreaType& outAreaType) const override final;

    Splitter* m_splitter;
    bool m_dragging : 1;

    void inputActionFinished();
    void dragging(bool flag);

    friend class SpliterSashInputAction;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
