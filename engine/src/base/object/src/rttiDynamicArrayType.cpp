/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\arrays #]
***/

#include "build.h"
#include "rttiArrayType.h"
#include "rttiDynamicArrayType.h"
#include "rttiTypeSystem.h"
#include "rttiArrayHelpers.h"
#include "rttiDataView.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"

namespace base
{
    namespace rtti
    {
        extern bool SkipString(const char*& ptr, const char* endPtr);


        namespace prv
        {

            const char* DynamicArrayType::TypePrefix = "array<";

            DynamicArrayType::DynamicArrayType(Type innerType)
                : IArrayType(FormatDynamicArrayTypeName(innerType->name()), innerType)
				, m_helper(nullptr)
            {
                m_traits.size = sizeof(BaseArray);
                m_traits.alignment = __alignof(BaseArray);
                m_traits.initializedFromZeroMem = true; // BaseArray is fine with zero memory init
                m_traits.requiresConstructor = true;
                m_traits.requiresDestructor = true;
            }

            DynamicArrayType::~DynamicArrayType()
            {}

            void DynamicArrayType::cacheTypeData()
            {
                IArrayType::cacheTypeData();

                ASSERT(innerType());
                ASSERT(innerType()->size() != 0);
                ASSERT(innerType()->alignment() != 0);

                m_helper = IArrayHelper::GetHelperForType(innerType());
            }

            void DynamicArrayType::construct(void *object) const
            {
                new (object) BaseArray();
            }


            void DynamicArrayType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
            {
                auto arr = (const BaseArray*)data;

                if (innerType()->name() == "char"_id || innerType()->name() == "char"_id)
                {
                    f.appendCEscaped((const char*)arr->data(), arr->size(), true);
                }
                else if (innerType()->name() == "uint8_t"_id || innerType()->name() == "uint8_t"_id)
                {
                    f.appendBase64(arr->data(), arr->size());
                }
                else
                {
                    auto stride = innerType()->size();
                    auto readPtr  = (const uint8_t*)arr->data();
                    for (uint32_t i = 0; i < arr->size(); ++i, readPtr += stride)
                    {
                        f << "[";
                        innerType()->printToText(f, readPtr, flags | PrintToFlag_TextSerializaitonArrayElement);
                        f << "]";
                    }
                }
            }

            bool ReadArrayValue(const char*& ptr, const char* endPtr, StringView& outValue)
            {
                int innerDepth = 0;
                int propertyDepth = 0;

                auto start  = ptr;
                while (ptr < endPtr)
                {
                    auto ch = *ptr++;

                    if (*ptr == '\"')
                    {
                        if (!SkipString(ptr, endPtr))
                            return false;
                        continue;
                    }

                    if (ch == ']')
                    {
                        if (innerDepth == 0 && propertyDepth == 0)
                        {
                            outValue = StringView(start, ptr - 1);
                            return true;
                        }
                        else
                        {
                            if (innerDepth == 0)
                                return false;
                            else
                                innerDepth -= 1;
                        }
                    }
                    else if (ch == '[')
                    {
                        innerDepth += 1;
                    }
                    else if (ch == '(')
                    {
                        propertyDepth += 1;
                    }
                    else if (ch == ')')
                    {
                        if (propertyDepth == 0)
                            return false;
                        else
                            propertyDepth -= 1;
                    }
                }

                return false;
            }

            DataViewResult DynamicArrayType::describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const
            {
                if (viewPath.empty())
                    outInfo.flags |= DataViewInfoFlagBit::DynamicArray;

                return IArrayType::describeDataView(viewPath, viewData, outInfo);
            }

            DataViewResult DynamicArrayType::writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const
            {
                return IArrayType::writeDataView(viewPath, viewData, sourceData, sourceType);
            }

            bool DynamicArrayType::parseFromString(StringView txt, void* data, uint32_t flags) const
            {
                auto arr  = (BaseArray*)data;
                if (innerType()->name() == "char"_id || innerType()->name() == "char"_id)
                {
                    // decode string data
                    uint32_t length = 0;
                    void* data = DecodeCString(txt.data(), txt.data() + txt.length(), length, POOL_MEM_BUFFER);
                    if (!data)
                        return false;

                    m_helper->destroy(arr, innerType());
                    m_helper->resize(arr, innerType(), length);
                    memcpy(arr->data(), data, length);
                    MemFree(data);
                }
                else if (innerType()->name() == "uint8_t"_id || innerType()->name() == "uint8_t"_id)
                {
                    uint32_t length = 0;
                    void* data = DecodeBase64(txt.data(), txt.data() + txt.length(), length);
                    if (!data)
                        return false;

                    m_helper->destroy(arr, innerType());
                    m_helper->resize(arr, innerType(), length);
                    memcpy(arr->data(), data, length);
                    MemFree(data);
                }
                else
                {
                    // count items
                    uint32_t itemCount = 0;
                    {
                        auto readPtr  = txt.data();
                        auto endPtr  = txt.data() + txt.length();
                        while (readPtr < endPtr)
                        {
                            auto ch = *readPtr++;
                            if (ch <= ' ')
                                continue;

                            if (ch != '[')
                                return false;

                            StringView value;
                            if (!ReadArrayValue(readPtr, endPtr, value))
                                return false;
                            itemCount += 1;
                        }
                    }

                    // prepare storage
                    m_helper->destroy(arr, innerType());
                    m_helper->resize(arr, innerType(), itemCount);
                    auto stride = innerType()->size();
                    auto writePtr  = (uint8_t*)arr->data();


                    // load items
                    auto readPtr  = txt.data();
                    auto endPtr  = txt.data() + txt.length();
                    while (readPtr < endPtr)
                    {
                        auto ch = *readPtr++;
                        if (ch <= ' ')
                            continue;

                        if (ch != '[')
                            return false;

                        StringView value;
                        if (!ReadArrayValue(readPtr, endPtr, value))
                            return false;

                        if (!innerType()->parseFromString(value, writePtr, flags | PrintToFlag_TextSerializaitonArrayElement))
                            return false;

                        writePtr += stride;
                    }
                }

                return true;
            }

            void DynamicArrayType::destruct(void *object) const
            {
                auto arr  = (BaseArray*)object;

                m_helper->destroy(arr, innerType());

                clearArrayElements(object);
            }
            
            ArrayMetaType DynamicArrayType::arrayMetaType() const
            {
                return ArrayMetaType::Dynamic;
            }

            uint32_t DynamicArrayType::arraySize(const void* data) const
            {
                auto arr  = (const BaseArray*)data;
                return arr->size();
            }

            uint32_t DynamicArrayType::arrayCapacity(const void* data) const
            {
				auto arr  = (const BaseArray*)data;
				return arr->capacity();
            }

			uint32_t DynamicArrayType::maxArrayCapacity(const void* data) const
			{
				return INDEX_MAX;
			}

            bool DynamicArrayType::canArrayBeResized() const
            {
                return true;
            }

            bool DynamicArrayType::clearArrayElements(void* data) const
            {
                auto arr  = (BaseArray*)data;
                m_helper->destroy(arr, innerType());
                return true;
            }

            bool DynamicArrayType::resizeArrayElements(void* data, uint32_t count) const
            {
                auto arr = (BaseArray*)data;
                m_helper->resize(arr, innerType(), count);
                return true;
            }

            bool DynamicArrayType::removeArrayElement(const void* data, uint32_t index) const
            {
                auto arr  = (BaseArray*)data;
                if (index >= arr->size())
                    return false;

                m_helper->eraseRange(arr, innerType(), index, 1);
                return true;
            }

            bool DynamicArrayType::createArrayElement(void* data, uint32_t index) const
            {
                auto arr  = (BaseArray*)data;
                if (index > arr->size())
                    return false;

                m_helper->insertRange(arr, innerType(), index, 1);
                return true;
            }

            const void* DynamicArrayType::arrayElementData(const void* data, uint32_t index) const
            {
                auto arr  = (const BaseArray*)data;

                auto size = arr->size();
                if (index >= size)
                    return nullptr;

                auto elemSize = innerType()->size();
                return (const uint8_t*)arr->data() + (index * elemSize);
            }

            void* DynamicArrayType::arrayElementData(void* data, uint32_t index) const
            {
                auto arr  = (BaseArray*)data;

                auto size = arr->size();
                if (index >= size)
                    return nullptr;

                auto elemSize = innerType()->size();
                return (uint8_t*)arr->data() + (index * elemSize);
            }

            //---

            Type DynamicArrayType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
            {
                StringID innerTypeName;
                if (!typeNameString.parseTypeName(innerTypeName))
                    return nullptr;

                if (!typeNameString.parseKeyword(">"))
                    return nullptr;

                if (auto innerType = typeSystem.findType(innerTypeName))
                    return Type(MemNew(DynamicArrayType, innerType));

                TRACE_ERROR("Unable to parse a array inner type from '{}'", innerTypeName);
                return nullptr;
            }

            //---

        } // prv

        extern StringID FormatDynamicArrayTypeName(StringID innerTypeName)
        {
            StringBuilder builder;
            builder.append(prv::DynamicArrayType::TypePrefix);
            builder.append(innerTypeName.c_str());
            builder.append(">");
            return StringID(builder.c_str());
        }

    } // rtti
} // base

