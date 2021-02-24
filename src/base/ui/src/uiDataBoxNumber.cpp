/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBoxNumber.h"
#include "uiEditBox.h"
#include "uiTrackBar.h"
#include "uiDragger.h"
#include "base/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxNumberText);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxNumber");
RTTI_END_TYPE();

static bool IsFloatingPointType(base::Type type)
{
    return (type == base::reflection::GetTypeObject<float>() || type == base::reflection::GetTypeObject<double>());
}

static base::StringBuf ValueToString(const base::rtti::DataHolder& data, int numDigits)
{
    base::StringBuilder txt;

    if (data.type() == base::reflection::GetTypeObject<float>())
    {
        txt.appendPreciseNumber(*(const float*)data.data(), numDigits);
    }
    else if (data.type() == base::reflection::GetTypeObject<double>())
    {
        txt.appendPreciseNumber(*(const double*)data.data(), numDigits);
    }
    else if (data.type() == base::reflection::GetTypeObject<char>())
    {
        txt.appendNumber(*(const char*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<short>())
    {
        txt.appendNumber(*(const short*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<int>())
    {
        txt.appendNumber(*(const int*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<int64_t>())
    {
        txt.appendNumber(*(const int64_t*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<uint8_t>())
    {
        txt.appendNumber(*(const uint8_t*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<uint16_t>())
    {
        txt.appendNumber(*(const uint16_t*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<uint32_t>())
    {
        txt.appendNumber(*(const uint32_t*)data.data());
    }
    else if (data.type() == base::reflection::GetTypeObject<uint64_t>())
    {
        txt.appendNumber(*(const uint64_t*)data.data());
    }

    return txt.toString();
}

template< typename T >
static bool ValueToInRange(T data, double rangeMin, double rangeMax)
{
    if (rangeMin >= std::numeric_limits<T>::lowest())
        if (data < (T)rangeMin)
            return false;

    if (rangeMax <= std::numeric_limits<T>::max())
        if (data > (T)rangeMax)
            return false;

    return true;
}

static bool ValueToInRange(const base::rtti::DataHolder& data, double rangeMin, double rangeMax)
{
    if (data.type() == base::reflection::GetTypeObject<float>())
        return ValueToInRange(*(const float*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<double>())
        return ValueToInRange(*(const double*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<char>())
        return ValueToInRange(*(const char*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<short>())
        return ValueToInRange(*(const short*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<int>())
        return ValueToInRange(*(const int*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<int64_t>())
        return ValueToInRange(*(const int64_t*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<uint8_t>())
        return ValueToInRange(*(const uint8_t*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<uint16_t>())
        return ValueToInRange(*(const uint16_t*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<uint32_t>())
        return ValueToInRange(*(const uint32_t*)data.data(), rangeMin, rangeMax);
    else if (data.type() == base::reflection::GetTypeObject<uint64_t>())
        return ValueToInRange(*(const uint64_t*)data.data(), rangeMin, rangeMax);

    return true;
}

static bool ValueFromString(base::StringView txt, base::rtti::DataHolder& data)
{
    if (data.type() == base::reflection::GetTypeObject<float>())
        return base::MatchResult::OK == txt.match(*(float*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<double>())
        return base::MatchResult::OK == txt.match(*(double*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<char>())
        return base::MatchResult::OK == txt.match(*(char*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<short>())
        return base::MatchResult::OK == txt.match(*(short*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<int>())
        return base::MatchResult::OK == txt.match(*(int*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<int64_t>())
        return base::MatchResult::OK == txt.match(*(int64_t*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<uint8_t>())
        return base::MatchResult::OK == txt.match(*(uint8_t*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<uint16_t>())
        return base::MatchResult::OK == txt.match(*(uint16_t*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<uint32_t>())
        return base::MatchResult::OK == txt.match(*(uint32_t*)data.data());
    else if (data.type() == base::reflection::GetTypeObject<uint64_t>())
        return base::MatchResult::OK == txt.match(*(uint64_t*)data.data());

    return false;
}

template< typename T >
static T CalcDragStep(int numDigits)
{
    if (std::is_floating_point<T>())
    {
    }
}

template< typename T >
static void CalcDragValueT(T base, int64_t steps, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool wrap, T& result)
{
    if (steps == 0)
    {
        result = base; // do not clamp
        return;
    }

    if (std::is_floating_point<T>::value)
    {
        /*if (rangeEnabled)
        {

        }*/

        static T StepValues[7] = { 1.0, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001 };
        const T step = StepValues[std::clamp<int>(numDigits, 0, ARRAY_COUNT(StepValues) - 1)];

        result = base + (step * steps);

        if (rangeEnabled)
        {
            if (result > rangeMax)
                result = rangeMax;
            else if (result < rangeMin)
                result = rangeMin;
        }
    }
    else if (steps > 0)
    {
        if (steps > std::numeric_limits<T>::max())
            steps = std::numeric_limits<T>::max();

        const auto oldResult = result;
        result += steps;

        if (result < oldResult)
            result = std::numeric_limits<T>::max();
    }
    else if (steps < 0)
    {
        if (steps < std::numeric_limits<T>::lowest())
            steps = std::numeric_limits<T>::lowest();

        const auto oldResult = result;
        result += steps;

        if (result > oldResult)
            result = std::numeric_limits<T>::lowest();
    }
}

static void CalcDragValue(const base::rtti::DataHolder& base, int64_t steps, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool wrap, base::rtti::DataHolder& result)
{
    if (base.type() == base::reflection::GetTypeObject<float>())
        CalcDragValueT(*(const float*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(float*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<double>())
        CalcDragValueT(*(const double*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(double*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<char>())
        CalcDragValueT(*(const char*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(char*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<short>())
        CalcDragValueT(*(const short*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(short*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<int>())
        CalcDragValueT(*(const int*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(int*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<int64_t>())
        CalcDragValueT(*(const int64_t*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(int64_t*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<uint8_t>())
        CalcDragValueT(*(const uint8_t*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(uint8_t*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<uint16_t>())
        CalcDragValueT(*(const uint16_t*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(uint16_t*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<uint32_t>())
        CalcDragValueT(*(const uint32_t*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(uint32_t*)result.data());
    else if (base.type() == base::reflection::GetTypeObject<uint64_t>())
        CalcDragValueT(*(const uint64_t*)base.data(), steps, numDigits, rangeEnabled, rangeMin, rangeMax, wrap, *(uint64_t*)result.data());
}

DataBoxNumberText::DataBoxNumberText(base::Type type, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool dragger, bool draggerWrap, base::StringView units)
    : m_type(type)
    , m_rangeEnabled(rangeEnabled)
    , m_typeInteger(!IsFloatingPointType(type))
    , m_numFractionalDigits(numDigits)
    , m_rangeMin(rangeMin)
    , m_rangeMax(rangeMax)
    , m_dragWrap(draggerWrap)
{
    layoutHorizontal();

    auto flags = ui::EditBoxFeatureFlags({ ui::EditBoxFeatureBit::AcceptsEnter, ui::EditBoxFeatureBit::NoBorder });

    m_editBox = createChild<EditBox>(flags);
    m_editBox->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_editBox->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    m_editBox->postfixText(units);

    m_editBox->bind(EVENT_TEXT_ACCEPTED, this) = [this]()
    {
        write();
    };

    if (dragger)
    {
        m_dragBox = createChild<Dragger>();
        m_dragBox->bind(EVENT_VALUE_DRAG_STARTED) = [this]() { dragStart(); };
        m_dragBox->bind(EVENT_VALUE_DRAG_STEP) = [this](int delta) { dragUpdate(delta); };
        m_dragBox->bind(EVENT_VALUE_DRAG_FINISHED) = [this]() { dragFinish(); };
        m_dragBox->bind(EVENT_VALUE_DRAG_CANCELED) = [this]() { dragCancel(); };
    }
}

void DataBoxNumberText::dragStart()
{
    m_preDragValue.reset();

    if (!readOnly())
    {
        base::rtti::DataHolder data(m_type);
        const auto ret = readValue(data.data(), data.type());
        if (ret.code == base::DataViewResultCode::OK)
            m_preDragValue = std::move(data);
        m_dragStepCounter = 0;
    }
}

void DataBoxNumberText::dragFinish()
{
    m_preDragValue.reset();
    handleValueChange();
}

void DataBoxNumberText::dragCancel()
{
    if (!m_preDragValue.empty())
    {
        writeValue(m_preDragValue.data(), m_preDragValue.type());
        m_preDragValue.reset();
    }

    handleValueChange();
}

void DataBoxNumberText::dragUpdate(int64_t numSteps)
{
    if (!m_preDragValue.empty())
    {
        m_dragStepCounter += numSteps;

        base::rtti::DataHolder value(m_type);
        CalcDragValue(m_preDragValue, m_dragStepCounter, m_numFractionalDigits, m_rangeEnabled, m_rangeMin, m_rangeMax, m_dragWrap, value);

        writeValue(value.data(), value.type());
    }

    handleValueChange();
}

void DataBoxNumberText::handleValueChange()
{
    m_editBox->enable(!readOnly());

    if (m_dragBox)
        m_dragBox->visibility(!readOnly());

    base::rtti::DataHolder holder(m_type);

    const auto ret = readValue(holder.data(), m_type);
    if (ret.code == base::DataViewResultCode::OK)
    {
        const auto txt = ValueToString(holder, m_numFractionalDigits);
        m_valueReadInRange = !m_rangeEnabled || ValueToInRange(holder, m_rangeMin, m_rangeMax);
        m_editBox->text(txt);
        m_editBox->enable(true);
    }
    else if (ret.code == base::DataViewResultCode::ErrorManyValues)
    {
        m_valueReadInRange = true;
        m_editBox->text("<many values>");
        m_editBox->enable(true);
    }
    else
    {
        m_editBox->text("<error>");
        m_editBox->enable(false);
    }
}

void DataBoxNumberText::cancelEdit()
{
    m_editBox->clearSelection();
    handleValueChange();
}

void DataBoxNumberText::enterEdit()
{
    m_editBox->selectWholeText();
    m_editBox->focus();
}

void DataBoxNumberText::write()
{
    auto txt = m_editBox->text();
    if (txt.empty())
        txt = "0";

    base::rtti::DataHolder holder(m_type);
    if (!ValueFromString(txt, holder))
        return;

    if (m_rangeEnabled && m_valueReadInRange)
        if (!ValueToInRange(holder, m_rangeMin, m_rangeMax))
            return;

    writeValue(holder.data(), holder.type());

    handleValueChange();
    m_editBox->selectWholeText();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxNumberTrackBar);
RTTI_END_TYPE();

DataBoxNumberTrackBar::DataBoxNumberTrackBar(base::Type type, int numDigits, double rangeMin, double rangeMax, bool editable)
    : m_type(type)
    , m_numFractionalDigits(numDigits)
    , m_rangeMin(rangeMin)
    , m_rangeMax(rangeMax)
{
    layoutHorizontal();

    m_bar = createChild<TrackBar>();
    m_bar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_bar->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    m_bar->range(rangeMin, rangeMax);
    m_bar->resolution(numDigits);
    m_bar->allowEditBox(editable);

    m_bar->bind(EVENT_TRACK_VALUE_CHANGED) = [this]()
    {
        write();
    };
}

void DataBoxNumberTrackBar::write()
{
    const auto val = m_bar->value();

    base::rtti::DataHolder holder(m_type);
    if (base::rtti::ConvertData(&val, base::reflection::GetTypeObject<double>(), holder.data(), holder.type()))
    {
        writeValue(holder.data(), m_type);
    }
}

void DataBoxNumberTrackBar::enterEdit()
{
    m_bar->focus();
}

void DataBoxNumberTrackBar::cancelEdit()
{
    m_bar->closeEditBox();
}

void DataBoxNumberTrackBar::handleValueChange()
{
    base::rtti::DataHolder holder(m_type);
    const auto ret = readValue(holder.data(), m_type);
    if (ret.code == base::DataViewResultCode::OK)
    {
        double val = 0.0;
        if (base::rtti::ConvertData(holder.data(), holder.type(), &val, base::reflection::GetTypeObject<double>()))
        {
            m_bar->value(val, false);
        }
    }
    else if (ret.code == base::DataViewResultCode::ErrorManyValues)
    {

    }
    else
    {
        // undetermined value
        // TODO
    }
}

//--

class DataBoxNumberFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxNumberFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
    {
        if (info.dataType == base::reflection::GetTypeObject<float>() ||
            info.dataType == base::reflection::GetTypeObject<double>() ||
            info.dataType == base::reflection::GetTypeObject<char>() ||
            info.dataType == base::reflection::GetTypeObject<short>() ||
            info.dataType == base::reflection::GetTypeObject<int>() ||
            info.dataType == base::reflection::GetTypeObject<int64_t>() ||
            info.dataType == base::reflection::GetTypeObject<uint8_t>() ||
            info.dataType == base::reflection::GetTypeObject<uint16_t>() ||
            info.dataType == base::reflection::GetTypeObject<uint32_t>() ||
            info.dataType == base::reflection::GetTypeObject<uint64_t>())
        {
            /*bool rangeEnabled = false;
            bool dragEnabled = false;
            bool dragWrapEnabled = false;
            bool trackBarEnabled = false;
            int numDigits = 2;
            double rangeMin = -DBL_MAX;
            double rangeMax = DBL_MAX;
            base::StringBuf units = "";

            for (const auto* metadata : info.metadata)
            {
                if (const auto dragMeta = base::rtti_cast<base::PropertyHasDragMetadata>(metadata))
                {
                    dragEnabled = true;
                    dragWrapEnabled = dragMeta->wrap;
                }
                else if (const auto trackMeta = base::rtti_cast<base::PropertyHasTrackbarMetadata>(metadata))
                {
                    trackBarEnabled = true;
                }
                else if (const auto rangeMeta = base::rtti_cast<base::PropertyNumberRangeMetadata>(metadata))
                {
                    rangeEnabled = true;
                    rangeMin = rangeMeta->min;
                    rangeMax = rangeMeta->max;
                }
                else if (const auto digitsMeta = base::rtti_cast<base::PropertyPrecisionDigitsMetadata>(metadata))
                {
                    numDigits = digitsMeta->digits;
                }
                else if (const auto unitsData = base::rtti_cast<base::PropertyUnitsMetadata>(metadata))
                {
                    units = unitsData->text;
                }
            }*/

            auto numDigits = info.editorData.m_digits;
            if (!IsFloatingPointType(info.dataType))
                numDigits = 0;

            if (info.editorData.m_widgetSlider)
                return base::RefNew<DataBoxNumberTrackBar>(info.dataType, numDigits, info.editorData.m_rangeMin, info.editorData.m_rangeMax, true);
            else
                return base::RefNew<DataBoxNumberText>(info.dataType, numDigits, info.editorData.rangeEnabled(), 
                    info.editorData.m_rangeMin, info.editorData.m_rangeMax, info.editorData.m_widgetDrag, info.editorData.m_widgetDragWrap, info.editorData.m_units);
        }

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxNumberFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE(ui)