/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiWindowPopup.h"
#include "uiSimpleTreeModel.h"
#include "base/canvas/include/canvas.h"

BEGIN_BOOMER_NAMESPACE(ui)

///----

DECLARE_UI_EVENT(EVENT_COLOR_SELECTED, base::Color)

///----

class ColorPickerLSBoxInputAction;

// helper element that displays the Lightness-Saturation rectangle for given hue
class BASE_UI_API ColorPickerLSBox : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ColorPickerLSBox, IElement);

public:
    ColorPickerLSBox();

    // generated OnColorChanged when L or S is changed

    inline base::Vector3 hls() const { return m_value; }

    void hls(const base::Vector3& val);

private:
    base::Vector3 m_value;

    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

	base::canvas::Geometry m_colorRectGeometry;
	base::canvas::Geometry m_cursorGeometry;

    Size m_rectSize;

    void recomputeGeometry(const Size& size);
    void updateFromPosition(const Position& pos);

	void rebuildCursorGeometry(const Size& size);

    friend class ColorPickerLSBoxInputAction;
};

///----

class ColorPickerHueBarInputAction;

// helper element for the color picker - the Hue bar
class BASE_UI_API ColorPickerHueBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ColorPickerHueBar, IElement);

public:
    ColorPickerHueBar();

    // generated OnColorChanged when H is changed

    inline float hue() const { return m_hue; }

    void hue(float h);

private:
    float m_hue = 0.0;

    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

	base::canvas::Geometry m_cursorGeometry;
	base::canvas::Geometry m_colorBarGeometry;
    Size m_rectSize;

    void recomputeGeometry(const Size& size);
    void updateFromPosition(const Position& pos);

	void rebuildCursorGeometry(const Size& size);

    friend class ColorPickerHueBarInputAction;
};

///----

// helper dialog that allows to select a color
class BASE_UI_API ColorPickerBox : public PopupWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(ColorPickerBox, PopupWindow);

public:
    ColorPickerBox(base::Color initialColor, bool editAlpha, base::StringView caption="");
    virtual ~ColorPickerBox();

    inline base::Color color() const { return m_currentColor; }

    // generated OnColorChanged when selected and general OnClosed when window itself is closed

private:
    base::Color m_initialColor;
    base::Color m_currentColor;

    TrackBar* m_barR;
    TrackBar* m_barG;
    TrackBar* m_barB;
    TrackBar* m_barA;

    ColorPickerLSBox* m_lsBox;
    ColorPickerHueBar* m_hueBar;

    IElement* m_previousColorBox;
    IElement* m_currentColorBox;

    TextLabel* m_valuesText;

    bool m_editAlpha = false;

    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

    void syncHue();
    void recomputeFromHLS();
    void recomputeFromColor(bool updateHSV = true);

    void sendColorChange();
};

///----

END_BOOMER_NAMESPACE(ui)