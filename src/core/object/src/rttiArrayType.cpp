/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiArrayType.h"

#include "serializationWriter.h"
#include "serializationReader.h"

#include "rttiDataView.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

IArrayType::IArrayType(StringID name, Type innerType)
    : IType(name)
    , m_innerType(innerType)
{
    m_traits.metaType = MetaType::Array;
}

IArrayType::~IArrayType()
{}

bool IArrayType::compare(const void* data1, const void* data2) const
{
    auto size1 = arraySize(data1);
    auto size2 = arraySize(data2);
    if (size1 != size2)
        return false;

    for (uint32_t i = 0; i < size1; ++i)
    {
        auto itemDataA  = arrayElementData(data1, i);
        auto itemDataB  = arrayElementData(data2, i);
        if (!m_innerType->compare(itemDataA, itemDataB))
            return false;
    }

    return true;
}

void IArrayType::copy(void* dest, const void* src) const
{
    auto size = arraySize(src);
    clearArrayElements(dest);

    for (uint32_t i = 0; i < size; ++i)
    {
        createArrayElement(dest, i);

        auto itemSrc  = arrayElementData(src, i);
        auto itemDest  = arrayElementData(dest, i);
        m_innerType->copy(itemDest, itemSrc);
    }
}

void IArrayType::writeBinary(TypeSerializationContext& typeContext, SerializationWriter& file, const void* data, const void* defaultData) const
{
    // save array size
    uint32_t size = arraySize(data);
    file.beginArray(size);

    // save elements
    const auto defaultSize = defaultData ? arraySize(defaultData) : 0;
    for (uint32_t i = 0; i < size; ++i)
    {
        auto elementData  = arrayElementData(data, i);
                
        const void* defaultElementData = nullptr;
        if (i < defaultSize)
            defaultElementData = arrayElementData(defaultData, i);

        m_innerType->writeBinary(typeContext, file, elementData, defaultElementData);
    }

    // exit array block
    file.endArray();
}

void IArrayType::readBinary(TypeSerializationContext& typeContext, SerializationReader& file, void* data) const
{
    // enter array block
    uint32_t size = 0;
    file.enterArray(size);

    // prepare array
    clearArrayElements(data);
    resizeArrayElements(data, size);

    // get element pointers
    if (size)
    {
        auto* writePtr = (uint8_t*)arrayElementData(data, 0);
        auto writeStride = m_innerType->size();

        // read elements
        auto capacity = maxArrayCapacity(data);
        for (uint32_t i = 0; i < size; ++i, writePtr += writeStride)
        {
            if (i < capacity)
            {
                m_innerType->readBinary(typeContext, file, writePtr);
            }
            else
            {
                // read and discard the elements that won't fit into the array
                DataHolder holder(m_innerType);
                m_innerType->readBinary(typeContext, file, holder.data());
            }
        }
    }

    // exit array
    file.leaveArray();
}

//--

void IArrayType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
{
    // save elements
    const auto defaultSize = defaultData ? arraySize(defaultData) : 0;
    const auto size = arraySize(data);
    for (uint32_t i = 0; i < size; ++i)
    {
        if (auto childNode = node.writeChild("element"))
        {
            auto elementData = arrayElementData(data, i);

            const void* defaultElementData = nullptr;
            if (i < defaultSize)
                defaultElementData = arrayElementData(defaultData, i);

            m_innerType->writeXML(typeContext, childNode, elementData, defaultElementData);
        }
    }
}

void IArrayType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
{
    InplaceArray<xml::Node, 64> childNodes;

    // collect child nodes
    for (xml::NodeIterator it(node, "element"); it; ++it)
        childNodes.emplaceBack(*it);

        // prepare array
    const auto size = childNodes.size();
    clearArrayElements(data);
    resizeArrayElements(data, size);

    // load elements
    if (size > 0)
    {
        // get element pointers
        auto* writePtr = (uint8_t*)arrayElementData(data, 0);
        auto writeStride = m_innerType->size();

        // read elements
        auto capacity = maxArrayCapacity(data);
        for (uint32_t i = 0; i < size; ++i, writePtr += writeStride)
        {
            if (i >= capacity)
                break;

            m_innerType->readXML(typeContext, childNodes[i], writePtr);
        }
    }
}

//--

DataViewResult IArrayType::describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const
{
    const auto orgViewPath = viewPath;

    if (viewPath.empty())
    {
        outInfo.dataPtr = viewData;
        outInfo.dataType = this;
        outInfo.arraySize = arraySize(viewData);
        outInfo.flags |= DataViewInfoFlagBit::LikeArray;

        return IType::describeDataView(viewPath, viewData, outInfo);
    }

    uint32_t index = 0;
    if (ParseArrayIndex(viewPath, index))
    {
        if (index >= arraySize(viewData))
            return DataViewResultCode::ErrorIndexOutOfRange;

        auto elementViewData  = arrayElementData(viewData, index);
        return m_innerType->describeDataView(viewPath, elementViewData, outInfo);
    }

    StringView propName;
    if (ParsePropertyName(viewPath, propName))
    {
        static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

        if (propName == "__size" || propName == "__capacity")
            return uintType->describeDataView(viewPath, viewData, outInfo);
    }

    return IType::describeDataView(orgViewPath, viewData, outInfo);
}

DataViewResult IArrayType::readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const
{
    const auto orgViewPath = viewPath;

    uint32_t index = 0;
    if (ParseArrayIndex(viewPath, index))
    {
        if (index >= arraySize(viewData))
            return DataViewResultCode::ErrorIndexOutOfRange;

        auto elementViewData  = arrayElementData(viewData, index);
        return m_innerType->readDataView(viewPath, elementViewData, targetData, targetType);
    }

    StringView propName;
    if (ParsePropertyName(viewPath, propName))
    {
        static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

        if (propName == "__size")
        { 
            const auto value = arraySize(viewData);
            if (!ConvertData(&value, uintType, targetData, targetType))
                return DataViewResultCode::ErrorTypeConversion;
            return DataViewResultCode::OK;
        }

        else if (propName == "__capacity")
        {
            const auto value = arrayCapacity(viewData);
            if (!ConvertData(&value, uintType, targetData, targetType))
                return DataViewResultCode::ErrorTypeConversion;
            return DataViewResultCode::OK;
        }
    }

    return IType::readDataView(orgViewPath, viewData, targetData, targetType);
}

DataViewResult IArrayType::writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const
{
    const auto orgViewPath = viewPath;

    uint32_t index = 0;
    if (ParseArrayIndex(viewPath, index))
    {
        if (index >= arraySize(viewData))
            return DataViewResultCode::ErrorIndexOutOfRange;

        auto elementViewData  = arrayElementData(viewData, index);
        return m_innerType->writeDataView(viewPath, elementViewData, sourceData, sourceType);
    }

    StringView propName;
    if (ParsePropertyName(viewPath, propName))
    {
        static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

        if (propName == "__size")
        {
            uint32_t newSize = 0;
            if (!ConvertData(sourceData, sourceType, &newSize, uintType))
                return DataViewResultCode::ErrorTypeConversion;
            return DataViewResultCode::ErrorIllegalAccess; // TODO!
        }

        else if (propName == "__capacity")
        {
            uint32_t newCapacity = 0;
            if (!ConvertData(sourceData, sourceType, &newCapacity, uintType))
                return DataViewResultCode::ErrorTypeConversion;

            return DataViewResultCode::ErrorIllegalAccess; // TODO!
        }
    }

    return IType::writeDataView(viewPath, viewData, sourceData, sourceType);
}

END_BOOMER_NAMESPACE()
