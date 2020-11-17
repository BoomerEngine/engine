/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#include "build.h"
#include "uiColorPickerBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiImage.h"
#include "uiRenderer.h"
#include "uiTrackBar.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "uiInputAction.h"
#include "uiNotebook.h"

namespace ui
{
    //---

    void RGBtoHSV(const base::Vector3& rgb, base::Vector3& hsv)
    {
        float flMax = std::max<float>(rgb.x, std::max<float>(rgb.y, rgb.z));
        float flMin = std::min<float>(rgb.x, std::min<float>(rgb.y, rgb.z));

        hsv.z = flMax;

        if (flMax != 0.0f)
            hsv.y = (flMax - flMin) / flMax;
        else
            hsv.y = 0.0f;

        if (hsv.y == 0.0f)
        {
            hsv.x = -1.0f;
        }
        else
        {
            float d = flMax - flMin;
            if (rgb.x == flMax)
                hsv.x = (rgb.y - rgb.z) / d;
            else if (rgb.y == flMax)
                hsv.x = 2.0f + (rgb.z - rgb.x) / d;
            else
                hsv.x = 4.0f + (rgb.x - rgb.y) / d;

            hsv.x *= 60.0f;
            if (hsv.x < 0.0f)
                hsv.x += 360.0f;
        }
    }

    void HSVtoRGB(const base::Vector3& hsv, base::Vector3& rgb)
    {
        if (hsv.y == 0.0F)
        {
            rgb.x = hsv.z;
            rgb.y = hsv.z;
            rgb.z = hsv.z;
            return;
        }

        float hue = hsv.x;
        if (hue == 360.0F)
            hue = 0.0F;

        hue /= 60.0F;

        int i = (int)std::floor(hue);
        float f = hue - i;    // fractional part
        float p = hsv.z * (1.0f - hsv.y);
        float q = hsv.z * (1.0f - hsv.y * f);
        float t = hsv.z * (1.0f - hsv.y * (1.0F - f));

        switch (i)
        {
            case 0: rgb = base::Vector3(hsv.z, t, p); break;
            case 1: rgb = base::Vector3(q, hsv.z, p); break;
            case 2: rgb = base::Vector3(p, hsv.z, t); break;
            case 3: rgb = base::Vector3(p, q, hsv.z); break;
            case 4: rgb = base::Vector3(t, p, hsv.z); break;
            case 5: rgb = base::Vector3(hsv.z, p, q); break;
        }
    }

    //---

    class ColorPickerLSBoxInputAction : public MouseInputAction
    {
    public:
        ColorPickerLSBoxInputAction(const base::RefPtr<ColorPickerLSBox>& box)
            : MouseInputAction(box, base::input::KeyCode::KEY_MOUSE0)
            , m_box(box)
        {
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            m_box->updateFromPosition(evt.absolutePosition().toVector());
            return InputActionResult();
        }

    private:
        base::RefPtr<ColorPickerLSBox> m_box;
    };

    //---

    RTTI_BEGIN_TYPE_CLASS(ColorPickerLSBox);
        RTTI_METADATA(ElementClassNameMetadata).name("ColorPickerLSBox");
    RTTI_END_TYPE();

    ColorPickerLSBox::ColorPickerLSBox()
        : m_value(0.0f, 1.0f, 1.0f)
    {
        hitTest(true);
        enableAutoExpand(false, false);
        allowFocusFromClick(true);
    }

    void ColorPickerLSBox::hls(const base::Vector3& val)
    {
        m_value = val;
        recomputeGeometry(m_rectSize);
    }

    void ColorPickerLSBox::renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        if (m_rectSize != drawArea.size())
            recomputeGeometry(drawArea.size());

        auto style = base::canvas::SolidColor(base::Color::WHITE);
        style.customUV = true;

        {
            base::canvas::Canvas::RawGeometry geom;
            geom.vertices = m_vertices.typedData();
            geom.indices = m_indices.typedData();
            geom.numIndices = m_indices.size();

            canvas.alphaMultiplier(mergedOpacity);
            canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);
            canvas.place(style, geom);
        }

        //if (m_hue >= 0.0f)
        {
            const float x = drawArea.size().x * m_value.y;
            const float y = drawArea.size().y * m_value.z;
            const float r = drawArea.size().y * 0.025f;

            base::canvas::GeometryBuilder builder;

            base::Vector3 circleColor;
            HSVtoRGB(base::Vector3(m_value.x, m_value.y, m_value.z), circleColor);

            if (m_value.z > 0.5f)
                builder.strokeColor(base::Color::BLACK);
            else
                builder.strokeColor(base::Color::WHITE);
            builder.fillColor(base::Color::FromVectorLinear(circleColor));
            builder.beginPath();
            builder.strokeWidth(2.0f);
            builder.circle(x, y, r);
            builder.fill();
            builder.stroke();

            canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);
            canvas.alphaMultiplier(mergedOpacity);
            canvas.place(builder);
        }

        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity);        
    }

    InputActionPtr ColorPickerLSBox::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked())
        {
            updateFromPosition(evt.absolutePosition().toVector());
            return base::RefNew<ColorPickerLSBoxInputAction>(AddRef(this));
        }

        return nullptr;
    }

    bool ColorPickerLSBox::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        outCursorType = base::input::CursorType::Hand;
        return true;
    }

    bool ColorPickerLSBox::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressedOrRepeated())
        {
            return true;
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    void ColorPickerLSBox::updateFromPosition(const Position& pos)
    {
        const auto sizeX = cachedDrawArea().size().x - 1.0f;
        const auto sizeY = cachedDrawArea().size().y - 1.0f;

        if (sizeX > 0.0f && sizeY > 0.0f)
        {
            m_value.z = std::clamp<float>(pos.y - cachedDrawArea().top(), 0.0f, sizeY) / sizeY;

            if (m_value.x >= 0.0f)
                m_value.y = std::clamp<float>(pos.x - cachedDrawArea().left(), 0.0f, sizeX) / sizeX;

            call(EVENT_COLOR_SELECTED);
        }
    }

    void ColorPickerLSBox::recomputeGeometry(const Size& size)
    {
        static const uint32_t GRID_SIZE = 32;

        m_rectSize = size;

        m_vertices.reset();
        m_indices.reset();
        m_vertices.resize((GRID_SIZE + 1) * (GRID_SIZE + 1));
        m_indices.resize(GRID_SIZE * GRID_SIZE * 6);

        {
            auto* writeV = m_vertices.typedData();

            for (uint32_t y = 0; y <= GRID_SIZE; ++y)
            {
                const float fy = y / (float)GRID_SIZE;
                for (uint32_t x = 0; x <= GRID_SIZE; ++x, ++writeV)
                {
                    const float fx = x / (float)GRID_SIZE;

                    base::Vector3 cornerColor;
                    if (m_value.x >= 0.0f)
                    {
                        HSVtoRGB(base::Vector3(m_value.x, fx, fy), cornerColor);
                    }
                    else
                    {
                        cornerColor.x = fy;
                        cornerColor.y = fy;
                        cornerColor.z = fy;
                    }

                    writeV->pos.x = fx * size.x;
                    writeV->pos.y = fy * size.y;
                    writeV->uv.x = fx;
                    writeV->uv.y = fy;
                    writeV->color = base::Color::FromVectorLinear(cornerColor);
                }
            }
        }

        {
            auto* writeI = m_indices.typedData();

            for (uint32_t y = 0; y < GRID_SIZE; ++y)
            {
                uint16_t prevLine = y * (GRID_SIZE + 1);
                uint16_t curLine = prevLine + (GRID_SIZE + 1);

                for (uint32_t x = 0; x < GRID_SIZE; ++x, writeI += 6, prevLine += 1, curLine += 1)
                {
                    writeI[0] = prevLine + 0;
                    writeI[1] = prevLine + 1;
                    writeI[2] = curLine + 0;

                    writeI[3] = prevLine + 1;
                    writeI[4] = curLine + 1;
                    writeI[5] = curLine + 0;
                }
            }
        }
    }

    //---

    class ColorPickerHueBarInputAction : public MouseInputAction
    {
    public:
        ColorPickerHueBarInputAction(const base::RefPtr<ColorPickerHueBar>& box)
            : MouseInputAction(box, base::input::KeyCode::KEY_MOUSE0)
            , m_box(box)
        {
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            m_box->updateFromPosition(evt.absolutePosition().toVector());
            return InputActionResult();
        }

    private:
        base::RefPtr<ColorPickerHueBar> m_box;
    };

    //---

    RTTI_BEGIN_TYPE_CLASS(ColorPickerHueBar);
        RTTI_METADATA(ElementClassNameMetadata).name("ColorPickerHueBar");
    RTTI_END_TYPE();

    ColorPickerHueBar::ColorPickerHueBar()
        : m_hue(0.0f)
    {
        hitTest(true);
        enableAutoExpand(false, false);
        allowFocusFromClick(true);
    }

    void ColorPickerHueBar::hue(float h)
    {
        m_hue = h;
    }

    void ColorPickerHueBar::renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        if (m_rectSize != drawArea.size())
            recomputeGeometry(drawArea.size());

        auto style = base::canvas::SolidColor(base::Color::WHITE);
        style.customUV = true;

        {
            base::canvas::Canvas::RawGeometry geom;
            geom.vertices = m_vertices.typedData();
            geom.indices = m_indices.typedData();
            geom.numIndices = m_indices.size();

            canvas.alphaMultiplier(mergedOpacity);
            canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);
            canvas.place(style, geom);
        }

        if (m_hue >= 0.0f)
        {
            const float x = drawArea.size().x * (m_hue / 360.0f);
            const float y = drawArea.size().y / 2.0f;
            const float r = drawArea.size().y * 0.2f;

            base::canvas::GeometryBuilder builder;

            base::Vector3 circleColor;
            HSVtoRGB(base::Vector3(m_hue, 1.0f, 1.0f), circleColor);

            builder.strokeColor(base::Color::BLACK);
            builder.fillColor(base::Color::FromVectorLinear(circleColor));
            builder.beginPath();
            builder.strokeWidth(2.0f);
            builder.circle(x, y, r);
            builder.fill();
            builder.stroke();

            canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);
            canvas.alphaMultiplier(mergedOpacity);
            canvas.place(builder);
        }

        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity);
    }

    InputActionPtr ColorPickerHueBar::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked())
        {
            updateFromPosition(evt.absolutePosition().toVector());
            return base::RefNew<ColorPickerHueBarInputAction>(AddRef(this));
        }

        return nullptr;
    }

    bool ColorPickerHueBar::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        outCursorType = base::input::CursorType::Hand;
        return true;
    }

    bool ColorPickerHueBar::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressedOrRepeated())
        {
            return true;
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    void ColorPickerHueBar::updateFromPosition(const Position& pos)
    {
        const auto sizeX = cachedDrawArea().size().x - 1.0f;

        if (sizeX > 0.0f)
        {
            m_hue = (std::clamp<float>(pos.x - cachedDrawArea().left(), 0.0f, sizeX) / sizeX) * 360.0f;
            call(EVENT_COLOR_SELECTED);
        }
    }

    void ColorPickerHueBar::recomputeGeometry(const Size& size)
    {
        static const uint32_t GRID_SIZE = 36;

        m_rectSize = size;

        m_vertices.reset();
        m_indices.reset();
        m_vertices.resize(2*(GRID_SIZE + 1));
        m_indices.resize(GRID_SIZE * 6);

        {
            auto* writeV = m_vertices.typedData();

            for (uint32_t y = 0; y <= 1; ++y)
            {
                const float fy = y;
                for (uint32_t x = 0; x <= GRID_SIZE; ++x, ++writeV)
                {
                    const float fx = x / (float)GRID_SIZE;

                    base::Vector3 cornerColor;
                    HSVtoRGB(base::Vector3(fx * 360.0f, 1.0f, 1.0f), cornerColor);

                    writeV->pos.x = fx * size.x;
                    writeV->pos.y = fy * size.y;
                    writeV->uv.x = fx;
                    writeV->uv.y = fy;
                    writeV->color = base::Color::FromVectorLinear(cornerColor);
                }
            }
        }

        {
            auto* writeI = m_indices.typedData();

            for (uint32_t y = 0; y < 1; ++y)
            {
                uint16_t prevLine = y * (GRID_SIZE + 1);
                uint16_t curLine = prevLine + (GRID_SIZE + 1);

                for (uint32_t x = 0; x < GRID_SIZE; ++x, writeI += 6, prevLine += 1, curLine += 1)
                {
                    writeI[0] = prevLine + 0;
                    writeI[1] = prevLine + 1;
                    writeI[2] = curLine + 0;

                    writeI[3] = prevLine + 1;
                    writeI[4] = curLine + 1;
                    writeI[5] = curLine + 0;
                }
            }
        }
    }


    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ColorPickerBox);
    RTTI_END_TYPE();

    ColorPickerBox::ColorPickerBox(base::Color initialColor, bool editAlpha, base::StringView caption /*= ""*/)
        : PopupWindow(ui::WindowFeatureFlagBit::DEFAULT_POPUP_DIALOG, "Select color")
        , m_initialColor(initialColor)
        , m_currentColor(initialColor)
        , m_editAlpha(editAlpha)
    {
        // color modes
        auto notebook = createChild<Notebook>();
        //notebook->customInitialSize(500, 500);
        notebook->expand();
        {
            // HLS tab
            auto tab = base::RefNew<IElement>();
            tab->layoutHorizontal();
            tab->customStyle("title"_id, base::StringBuf("[img:color_wheel] HLS"));
            notebook->attachTab(tab);

            // color widgets
            {
                auto widgets = tab->createChild();
                widgets->layoutVertical();
                widgets->customMargins(5, 5, 5, 5);

                // Light/Saturation area
                m_lsBox = widgets->createChild<ColorPickerLSBox>();
                m_lsBox->customMargins(0, 0, 0, 10);
                m_lsBox->customMinSize(256, 256);
                m_lsBox->bind(EVENT_COLOR_SELECTED) = [this]() { recomputeFromHLS(); sendColorChange(); };

                // Hue bar
                m_hueBar = widgets->createChild<ColorPickerHueBar>();
                m_hueBar->customMinSize(256, 32);
                m_hueBar->bind(EVENT_COLOR_SELECTED) = [this]() { syncHue(); recomputeFromHLS(); sendColorChange(); };
            }

            // color bars
            {
                auto bars = tab->createChild();
                bars->layoutVertical();
                bars->customMargins(10, 10, 10, 0);

                {
                    auto bar = bars->createChild();
                    bar->layoutHorizontal();
                    bar->customMargins(0, 0, 0, 5);
                    bar->createChild<TextLabel>("[size:+][b][color:#F00]R: ")->customVerticalAligment(ElementVerticalLayout::Middle);

                    m_barR = bar->createChild<TrackBar>();
                    m_barR->range(0, 255);
                    m_barR->resolution(0);
                    m_barR->allowEditBox(true);
                    m_barR->customStyle("width"_id, 256.0f);
                    m_barR->bind(EVENT_TRACK_VALUE_CHANGED) = [this]() { recomputeFromColor(); sendColorChange(); };
                    m_barR->value(m_currentColor.r);
                }

                {
                    auto bar = bars->createChild();
                    bar->layoutHorizontal();
                    bar->customMargins(0, 0, 0, 5);
                    bar->createChild<TextLabel>("[size:+][b][color:#0F0]G: ")->customVerticalAligment(ElementVerticalLayout::Middle);

                    m_barG = bar->createChild<TrackBar>();
                    m_barG->range(0, 255);
                    m_barG->resolution(0);
                    m_barG->allowEditBox(true);
                    m_barG->customStyle("width"_id, 256.0f);
                    m_barG->bind(EVENT_TRACK_VALUE_CHANGED) = [this]() { recomputeFromColor(); sendColorChange(); };
                    m_barG->value(m_currentColor.g);
                }

                {
                    auto bar = bars->createChild();
                    bar->layoutHorizontal();
                    bar->customMargins(0, 0, 0, 5);
                    bar->createChild<TextLabel>("[size:+][b][color:#00F]B: ")->customVerticalAligment(ElementVerticalLayout::Middle);

                    m_barB = bar->createChild<TrackBar>();
                    m_barB->range(0, 255);
                    m_barB->resolution(0);
                    m_barB->allowEditBox(true);
                    m_barB->customStyle("width"_id, 256.0f);
                    m_barB->bind(EVENT_TRACK_VALUE_CHANGED) = [this]() { recomputeFromColor(); sendColorChange(); };
                    m_barB->value(m_currentColor.b);
                }

                if (editAlpha)
                {
                    auto bar = bars->createChild();
                    bar->layoutHorizontal();
                    bar->customMargins(0, 0, 0, 5);
                    bar->createChild<TextLabel>("[size:+][b][color:#FFF]A: ")->customVerticalAligment(ElementVerticalLayout::Middle);

                    m_barA = bar->createChild<TrackBar>();
                    m_barA->range(0, 255);
                    m_barA->resolution(0);
                    m_barA->allowEditBox(true);
                    m_barA->customStyle("width"_id, 256.0f);
                    m_barA->bind(EVENT_TRACK_VALUE_CHANGED) = [this]() { recomputeFromColor(); sendColorChange(); };
                    m_barA->value(m_currentColor.a);
                }

                {
                    auto boxes = bars->createChild();
                    boxes->layoutHorizontal();
                    boxes->customMargins(5, 5, 5, 5);

                    m_previousColorBox = boxes->createChild();
                    m_previousColorBox->customMinSize(64, 64);

                    m_currentColorBox = boxes->createChild();
                    m_currentColorBox->customMinSize(64, 64);
                }

                {
                    m_valuesText = bars->createChild<TextLabel>("Hue: 0deg, Sat: 50%, Lightness: 50%");
                    m_valuesText->customMargins(5, 5, 5, 5);
                }
            }
        }

        {
            auto buttons = createChild();
            buttons->layoutHorizontal();
            buttons->customHorizontalAligment(ElementHorizontalLayout::Right);
            buttons->customPadding(3);

            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:undo] Undo");
                buttons->bind(EVENT_CLICKED) = [this]() {
                    m_barR->value(m_initialColor.r);
                    m_barG->value(m_initialColor.g);
                    m_barB->value(m_initialColor.b);
                    m_barA->value(m_initialColor.a);
                    recomputeFromColor();
                    sendColorChange();
                };
            }

            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Close");
                button->bind(EVENT_CLICKED) = [this]() { requestClose(); };
            }
        }

        recomputeFromColor();
    }
   
    ColorPickerBox::~ColorPickerBox()
    {}

    bool ColorPickerBox::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
        {
            m_currentColor = m_initialColor;
            sendColorChange();
            requestClose();
            return true;
        }
        else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RETURN)
        {
            requestClose();
            return true;
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    void ColorPickerBox::syncHue()
    {
        auto hls = m_lsBox->hls();
        hls.x = m_hueBar->hue();
        m_lsBox->hls(hls);
    }

    void ColorPickerBox::recomputeFromHLS()
    {
        base::Vector3 hsv;
        hsv = m_lsBox->hls();

        base::Vector3 rgb;
        HSVtoRGB(hsv, rgb);

        m_barR->value(rgb.x * 255.0f);
        m_barG->value(rgb.y * 255.0f);
        m_barB->value(rgb.z * 255.0f);

        recomputeFromColor(false);
    }

    void ColorPickerBox::recomputeFromColor(bool updateHSV)
    {
        m_currentColor.r = (uint8_t)std::clamp<float>(m_barR->value(), 0.0f, 255.0f);
        m_currentColor.g = (uint8_t)std::clamp<float>(m_barG->value(), 0.0f, 255.0f);
        m_currentColor.b = (uint8_t)std::clamp<float>(m_barB->value(), 0.0f, 255.0f);
        m_currentColor.a = (uint8_t)std::clamp<float>(m_barA->value(), 0.0f, 255.0f);

        m_previousColorBox->customBackgroundColor(m_initialColor);
        m_currentColorBox->customBackgroundColor(m_currentColor);

        base::Vector3 rgb;
        rgb.x = base::FloatFrom255(m_currentColor.r);
        rgb.y = base::FloatFrom255(m_currentColor.g);
        rgb.z = base::FloatFrom255(m_currentColor.b);

        base::Vector3 hsv;

        if (updateHSV)
        {
            RGBtoHSV(rgb, hsv);

            m_lsBox->hls(hsv);
            m_hueBar->hue(hsv.x);
        }
        else
        {
            hsv = m_lsBox->hls();
        }
        
        {
            base::StringBuilder txt;
            txt << "Hue: " << Prec(hsv.x, 1) << " deg\n";
            txt << "Saturation: " << Prec(hsv.y * 100.0f, 1) << "%\n";
            txt << "Lightness: " << Prec(hsv.z * 100.0f, 1) << "%\n";
            m_valuesText->text(txt.view());
        }
    }

    void ColorPickerBox::sendColorChange()
    {
        call(EVENT_COLOR_SELECTED, m_currentColor);
    }

} // ui

