/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_TRACK_VALUE_CHANGED, double);

//--

/// a simple track bar - used to modify a floating point value in a continous way
class BASE_UI_API TrackBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(TrackBar, IElement);

public:
    TrackBar();
    virtual ~TrackBar();

    // get the current value
    INLINE double value() const { return m_value; }

    //--

    // set the value range
    void range(double minValue, double maxRange);

    // set the resolution
    void resolution(int numFractionalDigits, double resolution = 0.0f);

    // set current value
    void value(double value, bool callEvent=false);

    // units text (displayed after the number)
    void units(base::StringView txt);

    // allow/disallow edit box
    void allowEditBox(bool isEditBoxAllowed);

    //--

    void closeEditBox();

private:
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override final;
    virtual bool handleCursorQuery(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::CursorType &outCursorType) const override;
    virtual bool handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta) override;

    double m_min;
    double m_max;
    double m_resolution;
    int m_numFractionDigits;
    bool m_allowEditBox;
    double m_value;
    base::StringBuf m_units;

    base::RefPtr<TextLabel> m_displayLabel;
    base::RefPtr<IElement> m_displayArea;
    base::RefPtr<EditBox> m_editBox;

    float m_dragMargin;

    double calcProportion() const;
    ElementArea calcTrackArea(const ElementArea& totalArea) const;

    bool isCloseToDragArea(const ElementArea& totalArea, const Position& p) const;

    void showEditBox();

    void updateDisplayLabel();
    void moveStep(int delta);

protected:
    virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
    virtual bool handleKeyEvent(const base::input::KeyEvent &evt) override;
    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;

    virtual InputActionPtr handleOverlayMouseClick(const ElementArea &area, const base::input::MouseClickEvent &evt) override;
    virtual void handleFocusLost() override;

    base::StringBuf formatDisplayText() const;
};

//--

END_BOOMER_NAMESPACE(ui)

