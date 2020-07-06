/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiArrayType.h"

#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"
#include "streamTextWriter.h"
#include "streamTextReader.h"
#include "rttiDataView.h"

namespace base
{
    namespace rtti
    {

        IArrayType::IArrayType(StringID name, Type innerType)
            : IType(name)
            , m_innertType(innerType)
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
                if (!m_innertType->compare(itemDataA, itemDataB))
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
                m_innertType->copy(itemDest, itemSrc);
            }
        }

        void IArrayType::calcCRC64(CRC64& crc, const void* data) const
        {
            auto size = arraySize(data);
            crc << size;

            for (uint32_t i = 0; i < size; ++i)
            {
                auto itemData = arrayElementData(data, i);
                m_innertType->calcCRC64(crc, itemData);
            }
        }

        bool IArrayType::writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            // save array size
            uint32_t size = arraySize(data);
            file.writeValue(size);

            // save elements
            for (uint32_t i = 0; i < size; ++i)
            {
                auto elementData  = arrayElementData(data, i);
                auto defaultElementData  = defaultData ? arrayElementData(defaultData, i) : nullptr;
                if (!m_innertType->writeBinary(typeContext, file, elementData, defaultElementData))
                    return false;
            }

            return true;
        }

        bool IArrayType::readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            // load array count
            uint32_t size = 0;
            file.readValue(size);

            // prepare array
            clearArrayElements(data);

            // read elements
			auto capacity = maxArrayCapacity(data);
			for (uint32_t i = 0; i < size; ++i)
            {
                if (i < capacity)
                {
                    createArrayElement(data, i);

                    auto elementData  = arrayElementData(data, i);
                    if (!m_innertType->readBinary(typeContext, file, elementData))
                        return false;
                }
                else
                {
                    // read and discard the elements that won't fit into the array
                    DataHolder holder(m_innertType);
                    if (!m_innertType->readBinary(typeContext, file, holder.data()))
                        return false;
                }
            }

            return true;
        }

        bool IArrayType::writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            // save elements
            auto size = arraySize(data);
            for (uint32_t i = 0; i < size; ++i)
            {
                auto elementData  = arrayElementData(data, i);
                auto defaultElementData  = defaultData ? arrayElementData(defaultData, i) : nullptr;

                stream.beginArrayElement();
                if (!m_innertType->writeText(typeContext, stream, elementData, defaultElementData))
                {
                    stream.endArrayElement();
                    return false;
                }

                stream.endArrayElement();
            }

            return true;
        }

        bool IArrayType::readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            // clear elements
            clearArrayElements(data);

			auto capacity = maxArrayCapacity(data);

            uint32_t index = 0;
            while (stream.beginArrayElement())
            {
                if (index < capacity)
                {
                    createArrayElement(data, index);

                    auto elementData  = arrayElementData(data, index);
                    if (!m_innertType->readText(typeContext, stream, elementData))
                    {
                        stream.endArrayElement();
                        return false;
                    }
                }

                stream.endArrayElement();
                ++index;
            }           

            // end array
            return true;
        }

        DataViewResult IArrayType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
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
                return m_innertType->describeDataView(viewPath, elementViewData, outInfo);
            }

            StringView<char> propName;
            if (ParsePropertyName(viewPath, propName))
            {
                static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

                if (propName == "__size" || propName == "__capacity")
                    return uintType->describeDataView(viewPath, viewData, outInfo);
            }

            return IType::describeDataView(orgViewPath, viewData, outInfo);
        }

        DataViewResult IArrayType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            const auto orgViewPath = viewPath;

            uint32_t index = 0;
            if (ParseArrayIndex(viewPath, index))
            {
                if (index >= arraySize(viewData))
                    return DataViewResultCode::ErrorIndexOutOfRange;

                auto elementViewData  = arrayElementData(viewData, index);
                return m_innertType->readDataView(viewPath, elementViewData, targetData, targetType);
            }

            StringView<char> propName;
            if (ParsePropertyName(viewPath, propName))
            {
                static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

                if (propName == "__size")
                { 
                    const auto value = arraySize(viewData);
                    if (!rtti::ConvertData(&value, uintType, targetData, targetType))
                        return DataViewResultCode::ErrorTypeConversion;
                    return DataViewResultCode::OK;
                }

                else if (propName == "__capacity")
                {
                    const auto value = arrayCapacity(viewData);
                    if (!rtti::ConvertData(&value, uintType, targetData, targetType))
                        return DataViewResultCode::ErrorTypeConversion;
                    return DataViewResultCode::OK;
                }
            }

            return IType::readDataView(orgViewPath, viewData, targetData, targetType);
        }

        DataViewResult IArrayType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            const auto orgViewPath = viewPath;

            uint32_t index = 0;
            if (ParseArrayIndex(viewPath, index))
            {
                if (index >= arraySize(viewData))
                    return DataViewResultCode::ErrorIndexOutOfRange;

                auto elementViewData  = arrayElementData(viewData, index);
                return m_innertType->writeDataView(viewPath, elementViewData, sourceData, sourceType);
            }

            StringView<char> propName;
            if (ParsePropertyName(viewPath, propName))
            {
                static const auto uintType = RTTI::GetInstance().findType("uint32_t"_id);

                if (propName == "__size")
                {
                    uint32_t newSize = 0;
                    if (!rtti::ConvertData(sourceData, sourceType, &newSize, uintType))
                        return DataViewResultCode::ErrorTypeConversion;
                    return DataViewResultCode::ErrorIllegalAccess; // TODO!
                }

                else if (propName == "__capacity")
                {
                    uint32_t newCapacity = 0;
                    if (!rtti::ConvertData(sourceData, sourceType, &newCapacity, uintType))
                        return DataViewResultCode::ErrorTypeConversion;

                    return DataViewResultCode::ErrorIllegalAccess; // TODO!
                }
            }

            return IType::writeDataView(viewPath, viewData, sourceData, sourceType);
        }

    } // rtti
} // base
