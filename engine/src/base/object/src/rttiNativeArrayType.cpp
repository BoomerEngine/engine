/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\arrays #]
***/

#include "build.h"
#include "rttiArrayType.h"
#include "rttiNativeArrayType.h"
#include "rttiTypeSystem.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"

namespace base
{
    namespace rtti
    {
        namespace prv
        {

            const char* NativeArrayType::TypePrefix = "[";

            NativeArrayType::NativeArrayType(Type innerType, uint32_t size)
                : IArrayType(FormatNativeArrayTypeName(innerType->name(), size), innerType)
                , m_elementCount(size)
            {
                m_traits.alignment = innerType->alignment();

                auto innerSize = innerType->size();
                auto innerAlignment = innerType->alignment();
                auto innerSeparation = std::max<uint32_t>(innerSize, innerAlignment);
                m_traits.size = innerSeparation * m_elementCount;

                m_traits.simpleCopyCompare = innerType->traits().simpleCopyCompare;
                m_traits.requiresDestructor = innerType->traits().requiresDestructor;
                m_traits.requiresConstructor = innerType->traits().requiresConstructor;
                m_traits.initializedFromZeroMem = innerType->traits().initializedFromZeroMem;
            }

            NativeArrayType::~NativeArrayType()
            {}

            void NativeArrayType::construct(void *object) const
            {
                for (uint32_t i = 0; i < m_elementCount; ++i)
                {
                    auto ptr  = arrayElementData(object, i);
                    innerType()->construct(ptr);
                }
            }

            void NativeArrayType::destruct(void *object) const
            {
                for (uint32_t i = 0; i < m_elementCount; ++i)
                {
                    auto ptr  = arrayElementData(object, i);
                    innerType()->destruct(ptr);
                }
            }

            void NativeArrayType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
            {
                if (innerType()->name() == "char"_id || innerType()->name() == "char"_id)
                {
                    // HACK: remove last zero since it's most likely a string
                    // TODO: analyze the data, if it looks like normal string that do it, otherwise don't
                    auto length = m_elementCount;
                    auto str  = (const char*)data;
                    if (str[m_elementCount - 1] == 0)
                        length -= 1;

                    f.appendCEscaped(str, length, flags | PrintToFlag_TextSerializaitonArrayElement);
                }
                else if (innerType()->name() == "uint8_t"_id || innerType()->name() == "uint8_t"_id)
                {
                    f.appendBase64(data, m_elementCount);
                }
                else
                {
                    auto stride = innerType()->size();
                    auto readPtr  = (const uint8_t*)data;
                    for (uint32_t i = 0; i < m_elementCount; ++i, readPtr += stride)
                    {
                        f << "[";
                        innerType()->printToText(f, readPtr, true);
                        f << "]";
                    }
                }
            }

            extern bool ReadArrayValue(const char*& ptr, const char* endPtr, StringView& outValue);

            bool NativeArrayType::parseFromString(StringView txt, void* data, uint32_t flags) const
            {
                if (innerType()->name() == "char"_id || innerType()->name() == "char"_id)
                {
                    uint32_t decodedSize = 0;
                    auto decodedData  = DecodeCString(txt.data(), txt.data() + txt.length(), decodedSize);
                    if (!decodedData)
                        return false;

                    auto maxCopySize = std::min<uint32_t>(m_elementCount - 1, decodedSize);
                    memcpy(data, decodedData, maxCopySize);
                    memset((uint8_t*)data + maxCopySize, 0, m_elementCount - maxCopySize);
                }
                else if (innerType()->name() == "uint8_t"_id || innerType()->name() == "uint8_t"_id)
                {
                    uint32_t decodedSize = 0;
                    void* decodedData = DecodeBase64(txt.data(), txt.data() + txt.length(), decodedSize);
                    if (!decodedData)
                        return false;

                    auto maxCopySize = std::min<uint32_t>(m_elementCount, decodedSize);
                    memcpy(data, decodedData, maxCopySize);
                    memset((uint8_t*)data + maxCopySize, 0, m_elementCount - maxCopySize);
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

                    // reinitialize array elements
                    auto stride = innerType()->size();
                    auto writePtr  = (uint8_t*)data;
                    auto writeEndPtr  = (uint8_t*)data + (stride * m_elementCount);

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

                        if (writePtr < writeEndPtr) // skip elements that do not fit
                        {
                            if (!innerType()->parseFromString(value, writePtr, flags | PrintToFlag_TextSerializaitonArrayElement))
                                return false;
                        }

                        writePtr += stride;
                    }

                    // clear rest
                    if (writePtr < writeEndPtr)
                    {
                        if (!innerType()->traits().requiresDestructor && !innerType()->traits().requiresConstructor)
                        {
                            auto bytes = (writeEndPtr - writePtr);
                            memset(writePtr, 0, bytes);
                        }
                        else
                        {
                            while (writePtr < writeEndPtr)
                            {
                                innerType()->destruct(writePtr);
                                innerType()->construct(writePtr);
                                writePtr += stride;
                            }
                        }
                    }
                }

                return true;
            }
            
            ArrayMetaType NativeArrayType::arrayMetaType() const
            {
                return ArrayMetaType::Native;
            }

            uint32_t NativeArrayType::arraySize(const void* data) const
            {
                return m_elementCount;
            }

            uint32_t NativeArrayType::arrayCapacity(const void* data) const
            {
                return m_elementCount;
            }

			uint32_t NativeArrayType::maxArrayCapacity(const void* data) const
			{
				return m_elementCount;
			}

            bool NativeArrayType::canArrayBeResized() const
            {
                return false;
            }

            bool NativeArrayType::clearArrayElements(void* data) const
            {
                return true;
            }

            bool NativeArrayType::resizeArrayElements(void* data, uint32_t count) const
            {
                return (count == m_elementCount);
            }

            bool NativeArrayType::removeArrayElement(const void* data, uint32_t index) const
            {
                return true;
            }

            bool NativeArrayType::createArrayElement(void* data, uint32_t index) const
            {
                return true;
            }

            const void* NativeArrayType::arrayElementData(const void* data, uint32_t index) const
            {
                if (index >= m_elementCount)
                    return nullptr;

                auto elemSize = innerType()->size();
                auto elemAlign = innerType()->alignment();
                auto elemSeparation = std::max<uint32_t>(elemSize, elemAlign);

                return base::OffsetPtr(data, index * elemSeparation);
            }

            void* NativeArrayType::arrayElementData(void* data, uint32_t index) const
            {
                if (index >= m_elementCount)
                    return nullptr;

                auto elemSize = innerType()->size();
                auto elemAlign = innerType()->alignment();
                auto elemSeparation = std::max<uint32_t>(elemSize, elemAlign);

                return base::OffsetPtr(data, index * elemSeparation);
            }

            //---

            Type NativeArrayType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
            {
                /*if (!typeNameString.parseKeyword("["))
                    return nullptr;*/

                uint32_t maxSize = 0;
                if (!typeNameString.parseUint32(maxSize))
                    return nullptr;

                if (!typeNameString.parseKeyword("]"))
                    return nullptr;

                StringID innerTypeName;
                if (!typeNameString.parseTypeName(innerTypeName))
                    return nullptr;

                if (auto innerType = typeSystem.findType(innerTypeName))
                    return Type(MemNew(NativeArrayType, innerType, maxSize));

                TRACE_ERROR("Unable to parse a array inner type from '{}'", innerTypeName);
                return nullptr;
            }

            //---

        } // prv

        extern StringID FormatNativeArrayTypeName(StringID innerTypeName, uint32_t maxSize)
        {
            StringBuilder builder;
            builder.appendf("[{}]", maxSize);
            builder.append(innerTypeName.c_str());
            return StringID(builder.c_str());
        }

    } // rtti
} // base

