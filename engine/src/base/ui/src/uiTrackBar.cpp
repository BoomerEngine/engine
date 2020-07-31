/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiTrackBar.h"
#include "uiEditBox.h"
#include "uiInputAction.h"
#include "base/containers/include/stringParser.h"
#include "uiTextValidation.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(TrackBar);
        RTTI_METADATA(ElementClassNameMetadata).name("TrackBar");
    RTTI_END_TYPE();

    TrackBar::TrackBar()
        : m_min(0.0)
        , m_max(1.0)
        , m_resolution(0.001)
        , m_numFractionDigits(3)
        , m_value(0.0)
        , m_dragMargin(4.0f)
        , m_allowEditBox(false)
    {
        hitTest(HitTestState::Enabled);

        m_displayLabel = createNamedChild<TextLabel>("ValueDisplay"_id);
        m_displayLabel->overlay(true);

        m_displayArea = createChildWithType<>("TrackBarArea"_id);
        m_displayArea->customHorizontalAligment(ElementHorizontalLayout::Expand);
        m_displayArea->customVerticalAligment(ElementVerticalLayout::Expand);

        updateDisplayLabel();
    }

    TrackBar::~TrackBar()
    {}

    void TrackBar::allowEditBox(bool isEditBoxAllowed)
    {
        if (m_allowEditBox != isEditBoxAllowed)
        {
            m_allowEditBox = isEditBoxAllowed;

            if (!m_allowEditBox)
                closeEditBox();
        }
    }

    base::StringBuf TrackBar::formatDisplayText() const
    {
        if (m_numFractionDigits > 0)
            return base::TempString("{}", Prec(m_value, m_numFractionDigits));
        else
            return base::TempString("{}", (int64_t)m_value);
    }

    void TrackBar::updateDisplayLabel()
    {
        auto text = formatDisplayText();
        m_displayLabel->text(base::TempString("{}{}", text, m_units));
    }

    void TrackBar::range(double minValue, double maxValue)
    {
        auto trueMin = std::min(minValue, maxValue);
        auto trueMax = std::max(minValue, maxValue);

        if (m_min != trueMin || m_max != trueMax)
        {
            m_min = trueMin;
            m_max = trueMax;
        }
    }

    void TrackBar::resolution(int numFractionalDigits, double resolution/* = 0.0f*/)
    {
        if (m_numFractionDigits != numFractionalDigits)
        {
            m_numFractionDigits = numFractionalDigits;
            updateDisplayLabel();
        }

        auto naturalResolution = (numFractionalDigits > 0) ? pow(10.0, -(float)numFractionalDigits) : 1.0f;
        if (resolution != 0.0)
            m_resolution = resolution;
        else
            m_resolution = naturalResolution;
    }

    void TrackBar::value(double value, bool callEvent)
    {
        auto safeValue = std::clamp(value, m_min, m_max);
        if (safeValue != m_value)
        {
            m_value = safeValue;

            updateDisplayLabel();

            if (callEvent)
                call(EVENT_TRACK_VALUE_CHANGED, m_value);
        }
    }

    void TrackBar::units(base::StringView<char> txt)
    {
        if (m_units != txt)
        { 
            m_units = base::StringBuf(txt);
            updateDisplayLabel();

            if (m_editBox)
                m_editBox->postfixText(txt);
        }
    }

    double TrackBar::calcProportion() const
    {
        if (m_value <= m_min)
            return 0.0;
        else if (m_value >= m_max)
            return 1.0;
        else
            return (m_value - m_min) / (m_max - m_min);
    }

    ElementArea TrackBar::calcTrackArea(const ElementArea& totalArea) const
    {
        auto pos = totalArea.left() + (float)(totalArea.size().x * calcProportion());
        return ElementArea(totalArea.left(), totalArea.top(), pos, totalArea.bottom());
    }

    void TrackBar::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
    {
        // if we have the text box active collect only it
        if (m_editBox)
        {
            // use the whole area for the text box
            outArrangedChildren.add(m_editBox, innerArea, clipArea);
        }
        else
        {
            // compute the placement of the track area
            auto trackArea = calcTrackArea(innerArea);

            // place the track area exactly in the track area
            outArrangedChildren.add(m_displayArea, trackArea, clipArea);

            // place the text area so it can be seen properly
            /*auto textSize = m_displayLabel->cachedLayoutParams().calcTotalSize();
            auto textArea = trackArea;
            if (trackArea.right() < textSize.x)
                textArea = ElementArea(trackArea.right(), trackArea.top(), innerArea.right(), trackArea.bottom());*/

            // place the text in a safe space
            outArrangedChildren.add(m_displayLabel, innerArea, clipArea);
        }
    }

    bool TrackBar::isCloseToDragArea(const ElementArea& totalArea, const Position& p) const
    {
        auto trackArea = calcTrackArea(totalArea);
        auto dist = trackArea.right() - p.x;
        return fabs(dist) <= m_dragMargin;
    }

    namespace helper
    {
        class TrackBarDragAction : public MouseInputAction
        {
        public:
            TrackBarDragAction(TrackBar* ptr, const Position& pos, float deltaPos, double minRange, double maxRange, double resolution)
                : MouseInputAction(ptr, base::input::KeyCode::KEY_MOUSE0, true)
                , m_trackBar(ptr)
                , m_startPos(pos)
                , m_deltaPos(deltaPos)
                , m_minRange(minRange)
                , m_maxRange(maxRange)
                , m_resolution(resolution)
            {
                m_startValue = ptr->value();
                m_currentValue = ptr->value();
            }

            virtual void onCanceled() override
            {
                if (auto bar = m_trackBar.lock())
                    bar->value(m_startValue);
            }

            virtual bool allowsHoverTracking() const override
            {
                return false;
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
            {
                if (auto bar = m_trackBar.lock())
                {
                    auto trackPos = (evt.absolutePosition().x + m_deltaPos);
                    auto trackArea = bar->cachedDrawArea();
                    if (trackArea.size().x >= 1.0f)
                    {
                        auto trackRelativePos = (trackPos - trackArea.left()) / (double)trackArea.size().x;
                        auto trackValue = m_minRange + trackRelativePos * (m_maxRange - m_minRange);
                        auto trackSnappedValue = base::Snap(trackValue, m_resolution);
                        if (trackSnappedValue != m_currentValue)
                        {
                            m_currentValue = trackSnappedValue;
                            bar->value(trackSnappedValue, true);
                        }
                    }
                }

                return InputActionResult(); // keep active
            }

        private:
            base::RefWeakPtr<TrackBar> m_trackBar;
            Position m_startPos;
            float m_deltaPos;

            double m_startValue;
            double m_currentValue;

            double m_minRange;
            double m_maxRange;
            double m_resolution;
        };

    } // helper

    void TrackBar::handleFocusLost()
    {
        //closeEditBox();
        TBaseClass::handleFocusLost();
    }

    InputActionPtr TrackBar::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        // are we close to the drag area?
        if (evt.leftClicked() && isCloseToDragArea(area, evt.absolutePosition().toVector()))
        {
            auto trackArea = calcTrackArea(area);
            auto deltaPos = trackArea.right() - (float)evt.absolutePosition().x;
            return base::CreateSharedPtr<helper::TrackBarDragAction>(this, evt.absolutePosition().toVector(), deltaPos, m_min, m_max, m_resolution);
        }

        // nothing
        return TBaseClass::handleMouseClick(area, evt);
    }

    bool TrackBar::handleCursorQuery(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::CursorType &outCursorType) const
    {
        // show the sizer cursor when close to drag area
        if (isCloseToDragArea(area, absolutePosition))
        {
            outCursorType = base::input::CursorType::SizeWE;
            return true;
        }

        // pass to base class
        return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
    }

    bool TrackBar::handleKeyEvent(const base::input::KeyEvent &evt)
    {
        auto stepSize = evt.keyMask().isCtrlDown() ? 1 : (evt.keyMask().isShiftDown() ? 100 : 10);
        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_LEFT)
        {
            moveStep(-stepSize);
            return true;
        }
        else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RIGHT)
        {
            moveStep(stepSize);
            return true;
        }

        if (m_allowEditBox)
        {
            if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
            {
                closeEditBox();
                return true;
            }
            else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RETURN)
            {
                if (!m_editBox)
                    showEditBox();
                return true;
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    bool TrackBar::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "min")
        {
            double val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            m_min = val;
            return true;
        }
        else if (name == "max")
        {
            double val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            m_max = val;
            return true;
        }
        else if (name == "resolution")
        {
            double val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            resolution(m_numFractionDigits, val);
            return true;
        }
        else if (name == "digits" || name == "numDigits")
        {
            int val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            resolution(val, m_resolution);
            return true;
        }
        else if (name == "value")
        {
            double val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            this->value(val, false);
            return true;
        }
        else if (name == "edit" || name == "allowEditBox")
        {
            bool val = 0;
            if (base::MatchResult::OK != value.match(val))
                return false;
            allowEditBox(val);
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    void TrackBar::moveStep(int delta)
    {
        if (m_resolution > 0.0f)
        {
            auto snappedValue = base::Snap(m_value, m_resolution);
            if (delta < 0)
            {
                auto newValue = std::max<double>(m_min, snappedValue + m_resolution * delta);
                value(newValue);
            }
            else if (delta > 0)
            {
                auto newValue = std::min<double>(m_max, snappedValue + m_resolution * delta);
                value(newValue);
            }
        }
    }

    bool TrackBar::handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta)
    {
       if (delta < 0.0f && m_value > m_min)
        {
            moveStep(-1);
            return true;
        }
        else if (delta > 0.0f && m_value < m_max)
        {
            moveStep(+1);
            return true;
        }

        return false;
    }

    InputActionPtr TrackBar::handleOverlayMouseClick(const ElementArea &area, const base::input::MouseClickEvent &evt)
    {
        if (evt.leftDoubleClicked() && m_allowEditBox)
        {
            showEditBox();
            return IInputAction::CONSUME();
        }

        return TBaseClass::handleOverlayMouseClick(area, evt);
    }

    void TrackBar::showEditBox()
    {
        if (!m_editBox)
        {
            m_editBox = createChild<EditBox>(EditBoxFeatureFlags({ EditBoxFeatureBit::AcceptsEnter, EditBoxFeatureBit::NoBorder }));
            m_editBox->customHorizontalAligment(ElementHorizontalLayout::Expand);
            m_editBox->customVerticalAligment(ElementVerticalLayout::Expand);

            if (m_numFractionDigits == 0)
                m_editBox->validation(MakeIntegerNumbersInRangeValidationFunction((int64_t)m_min, (int64_t)m_max));
            else
                m_editBox->validation(MakeRealNumbersInRangeValidationFunction(m_min, m_max));

            m_editBox->text(formatDisplayText());
            m_editBox->selectWholeText();
            m_editBox->focus();

            m_editBox->bind(EVENT_TEXT_ACCEPTED, this) = [this](EditBox* box)
            {
                auto valueString = box->text();

                double number = value();
                if (base::StringParser(valueString).parseDouble(number))
                {
                    closeEditBox();
                    value(number, true);
                }
            };
        }
    }

    void TrackBar::closeEditBox()
    {
        if (m_editBox)
        {
            detachChild(m_editBox);
            m_editBox.reset();
        }
    }

    //--

} // ui

