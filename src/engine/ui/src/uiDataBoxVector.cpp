/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBoxNumber.h"
#include "uiTextLabel.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

struct CompInfo
{
    StringView path;
    StringView caption;
};

static CompInfo VECTOR_COMPS[4] = { {"x", " X "}, {"y", " Y "}, {"z", " Z "}, {"w", " W "} };
static CompInfo ANGLE_COMPS[3] = { {"roll", " R "}, {"pitch", " P "}, {"yaw", " Y "} };

/// Vector3 editor
class DataBoxVector : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxVector, IDataBox);

public:
    DataBoxVector(Type type, int numComps, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool dragger, bool draggerWrap, StringView units, const CompInfo* comps, bool vertical)
        : m_comps(comps)
    {
        if (vertical)
        {
            layoutVertical();

            if (numComps >= 1)
            {
                auto inner = createChild<IElement>();
                inner->layoutHorizontal();

                inner->createNamedChild<TextLabel>("X"_id, comps[0].caption);
                m_componentX = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 2)
            {
                auto inner = createChild<IElement>();
                inner->layoutHorizontal();

                inner->createNamedChild<TextLabel>("Y"_id, comps[1].caption);
                m_componentY = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 3)
            {
                auto inner = createChild<IElement>();
                inner->layoutHorizontal();

                inner->createNamedChild<TextLabel>("Z"_id, comps[2].caption);
                m_componentZ = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 4)
            {
                auto inner = createChild<IElement>();
                inner->layoutHorizontal();

                inner->createNamedChild<TextLabel>("W"_id, comps[3].caption);
                m_componentW = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }
        }
        else
        {
            layoutHorizontal();

            if (numComps >= 1)
            {
                createNamedChild<TextLabel>("X"_id, comps[0].caption);
                m_componentX = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 2)
            {
                createNamedChild<TextLabel>("Y"_id, comps[1].caption);
                m_componentY = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 3)
            {
                createNamedChild<TextLabel>("Z"_id, comps[2].caption);
                m_componentZ = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }

            if (numComps >= 4)
            {
                createNamedChild<TextLabel>("W"_id, comps[3].caption);
                m_componentW = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, draggerWrap, units);
            }
        }
    }

    virtual void bindData(const DataViewPtr& data, const StringBuf& path, bool readOnly /*= false*/) override
    {
        if (m_componentX)
            m_componentX->bindData(data, TempString("{}.{}", path, m_comps[0].path), readOnly);
        if (m_componentY)
            m_componentY->bindData(data, TempString("{}.{}", path, m_comps[1].path), readOnly);
        if (m_componentZ)
            m_componentZ->bindData(data, TempString("{}.{}", path, m_comps[2].path), readOnly);
        if (m_componentW)
            m_componentW->bindData(data, TempString("{}.{}", path, m_comps[3].path), readOnly);

        TBaseClass::bindData(data, path, readOnly);
    }

    virtual void bindActionHistory(ActionHistory* ah)
    {
        if (m_componentX)
            m_componentX->bindActionHistory(ah);
        if (m_componentY)
            m_componentY->bindActionHistory(ah);
        if (m_componentZ)
            m_componentZ->bindActionHistory(ah);
        if (m_componentW)
            m_componentW->bindActionHistory(ah);

        TBaseClass::bindActionHistory(ah);
    }

    virtual void enterEdit() override
    {
        if (m_componentX)
            m_componentX->enterEdit();
    }

    virtual void cancelEdit() override
    {
        if (m_componentX)
            m_componentX->cancelEdit();
        if (m_componentY)
            m_componentY->cancelEdit();
        if (m_componentZ)
            m_componentZ->cancelEdit();
    }

    virtual void handleValueChange() override
    {
        if (m_componentX)
            m_componentX->handleValueChange();
        if (m_componentY)
            m_componentY->handleValueChange();
        if (m_componentZ)
            m_componentZ->handleValueChange();
        if (m_componentW)
            m_componentW->handleValueChange();
    }

    virtual bool canExpandChildren() const override
    {
        return false;
    }

protected:
    DataBoxPtr m_componentX;
    DataBoxPtr m_componentY;
    DataBoxPtr m_componentZ;
    DataBoxPtr m_componentW;

    const CompInfo* m_comps;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxVector);
    RTTI_METADATA(ElementClassNameMetadata).name("DataBoxVector");
RTTI_END_TYPE();

//--

class DataBoxVectorFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxVectorFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const DataViewInfo& info) const override
    {
        const auto verticalLayout = info.flags.test(DataViewInfoFlagBit::VerticalEditor);

        if (info.dataType == GetTypeObject<Vector2>()
            || info.dataType == GetTypeObject<Vector3>()
            || info.dataType == GetTypeObject<Vector4>())
        {
            int numComps = 2;

            if (info.dataType == GetTypeObject<Vector3>())
                numComps = 3;
            else if (info.dataType == GetTypeObject<Vector4>())
                numComps = 4;

            /*bool rangeEnabled = false;
            bool dragEnabled = false;
            bool dragWrapEnabled = false;
            int numDigits = 2;
            double rangeMin = -DBL_MAX;
            double rangeMax = DBL_MAX;
            StringBuf units = "";

            for (const auto* metadata : info.metadata)
            {
                if (const auto dragMeta = rtti_cast<PropertyHasDragMetadata>(metadata))
                {
                    dragEnabled = true;
                    dragWrapEnabled = dragMeta->wrap;
                }
                else if (const auto rangeMeta = rtti_cast<PropertyNumberRangeMetadata>(metadata))
                {
                    rangeEnabled = true;
                    rangeMin = rangeMeta->min;
                    rangeMax = rangeMeta->max;
                }
                else if (const auto digitsMeta = rtti_cast<PropertyPrecisionDigitsMetadata>(metadata))
                {
                    numDigits = digitsMeta->digits;
                }
                else if (const auto unitsData = rtti_cast<PropertyUnitsMetadata>(metadata))
                {
                    units = unitsData->text;
                }
            }*/

            const auto& ed = info.editorData;
            return RefNew<DataBoxVector>(GetTypeObject<float>(), numComps, ed.m_digits, ed.rangeEnabled(), ed.m_rangeMin, ed.m_rangeMax, ed.m_widgetDrag, ed.m_widgetDragWrap, ed.m_units, VECTOR_COMPS, verticalLayout);
        }
        else if (info.dataType == GetTypeObject<Point>())
        {
            /*bool rangeEnabled = false;
            bool dragEnabled = false;
            bool dragWrapEnabled = false;
            double rangeMin = -DBL_MAX;
            double rangeMax = DBL_MAX;
            StringBuf units = "";

            for (const auto* metadata : info.metadata)
            {
                if (const auto dragMeta = rtti_cast<PropertyHasDragMetadata>(metadata))
                {
                    dragEnabled = true;
                    dragWrapEnabled = dragMeta->wrap;
                }
                else if (const auto rangeMeta = rtti_cast<PropertyNumberRangeMetadata>(metadata))
                {
                    rangeEnabled = true;
                    rangeMin = rangeMeta->min;
                    rangeMax = rangeMeta->max;
                }
                else if (const auto unitsData = rtti_cast<PropertyUnitsMetadata>(metadata))
                {
                    units = unitsData->text;
                }
            }*/

            const auto& ed = info.editorData;
            return RefNew<DataBoxVector>(GetTypeObject<int>(), 2, 0, ed.rangeEnabled(), ed.m_rangeMin, ed.m_rangeMax, ed.m_widgetDrag, ed.m_widgetDragWrap, ed.m_units, VECTOR_COMPS, verticalLayout);
        }
        else if (info.dataType == GetTypeObject<Angles>())
        {
            /*bool rangeEnabled = true;
            double rangeMin = -360.0f;
            double rangeMax = 360.0f;
            bool dragEnabled = true;
            bool dragWrapEnabled = true;
            StringBuf units = " deg";
            int numDigits = 2;

            for (const auto* metadata : info.metadata)
            {
                if (const auto dragMeta = rtti_cast<PropertyHasDragMetadata>(metadata))
                {
                    dragEnabled = true;
                    dragWrapEnabled = dragMeta->wrap;
                }
                else if (const auto rangeMeta = rtti_cast<PropertyNumberRangeMetadata>(metadata))
                {
                    rangeEnabled = true;
                    rangeMin = rangeMeta->min;
                    rangeMax = rangeMeta->max;
                }
                else if (const auto digitsMeta = rtti_cast<PropertyPrecisionDigitsMetadata>(metadata))
                {
                    numDigits = digitsMeta->digits;
                }
                else if (const auto unitsData = rtti_cast<PropertyUnitsMetadata>(metadata))
                {
                    units = unitsData->text;
                }
            }*/

            const auto& ed = info.editorData;
            return RefNew<DataBoxVector>(GetTypeObject<float>(), 3, ed.m_digits, ed.rangeEnabled(), ed.m_rangeMin, ed.m_rangeMax, ed.m_widgetDrag, ed.m_widgetDragWrap, ed.m_units, ANGLE_COMPS, verticalLayout);
        }

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxVectorFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
