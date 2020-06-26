/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiBitfieldType.h"

#include "streamTextWriter.h"
#include "streamTextReader.h"
#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"
#include "rttiDataView.h"
#include "dataView.h"

namespace base
{
    namespace rtti
    {

        BitfieldType::BitfieldType(StringID name, uint32_t size, uint64_t nativeHash)
            : IType(name)
        {
            DEBUG_CHECK_EX(size == 1 || size == 2 || size == 4 || size == 8, "Unsupported bitfield size");

            m_traits.metaType = MetaType::Bitfield;
            m_traits.size = size;
            m_traits.alignment = 1;
            m_traits.nativeHash = nativeHash;
            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = false;
            m_traits.requiresDestructor = false;
            m_traits.simpleCopyCompare = true;
        }

        INLINE static bool IsPowerOfTwo(uint64_t x)
        {
            return (x & (x - 1)) == 0;
        }

        uint64_t BitfieldType::GetBitMask(uint8_t bitIndex)
        {
            return 1ULL << bitIndex;
        }

        uint8_t BitfieldType::GetBitIndex(uint64_t bitMask)
        {
            DEBUG_CHECK_EX(IsPowerOfTwo(bitMask), "Bitfield flags must be power of two values");
            DEBUG_CHECK_EX(bitMask != 0, "Invalid bit mask");
            return (uint8_t)__builtin_ctzll(bitMask);
        }

        void BitfieldType::add(StringID name, uint64_t bitMask)
        {
            auto bitIndex = GetBitIndex(bitMask);
            DEBUG_CHECK_EX(m_flagNames[bitIndex].empty(), "Bit flag alread defined");
            m_flagNames[bitIndex] = name;
            m_flagBitIndices[name] = bitIndex;
        }

        void BitfieldType::options(Array<StringID>& outInfos) const
        {
            for (auto key : m_flagBitIndices.keys())
                outInfos.pushBack(key);
        }

        bool BitfieldType::findFlag(StringID name, uint64_t& outBitMaks) const
        {
            uint32_t index = 0;
            if (m_flagBitIndices.find(name, index))
            {
                outBitMaks = GetBitMask(index);
                return true;
            }

            return false;
        }

        bool BitfieldType::findName(uint64_t bitMask, StringID& outName) const
        {
            if (!bitMask || !IsPowerOfTwo(bitMask))
                return false;

            auto bitIndex = GetBitIndex(bitMask);
            if (m_flagNames[bitIndex].empty())
                return false;

            outName = m_flagNames[bitIndex];
            return true;
        }

        bool BitfieldType::compare(const void* data1, const void* data2) const
        {
            uint64_t  a, b;

            readUint64(data1, a);
            readUint64(data2, b);

            return (a == b);
        }

        void BitfieldType::copy(void* dest, const void* src) const
        {
            uint64_t val;
            readUint64(src, val);
            writeUint64(dest, val);
        }

        void BitfieldType::calcCRC64(CRC64& crc, const void* data) const
        {
            uint64_t val;
            readUint64(data, val);
            crc << val;
        }

        bool BitfieldType::writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            uint64_t val = 0;
            readUint64(data, val);

            // write flag name for each set bit
            auto maxBits = (uint8_t)(8 * size());
            for (uint8_t i = 0; i < maxBits; ++i)
            {
                if (val & GetBitMask(i))
                {
                    if (m_flagNames[i].empty())
                    {
                        TRACE_ERROR("Missing flag name for flag {} in {}, the value would be lost", i, name());
                        return false;
                    }

                    file.writeName(m_flagNames[i]);
                }
            }

            // guard
            file.writeName(StringID());
            return true;
        }

        bool BitfieldType::readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            uint64_t val = 0;

            for (;;)
            {
                auto flagName = file.readName();
                if (flagName.empty())
                    break;

                uint64_t flagBitMask = 0;
                if (!findFlag(flagName, flagBitMask))
                {
                    TRACE_ERROR("Failed to find flag value for flag '{}' in bitfield '{}'", flagName, name());
                    return false;
                }

                val |= flagBitMask;
            }

            writeUint64(data, val);
            return true;
        }

        bool BitfieldType::writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            uint64_t val = 0;
            readUint64(data, val);

            if (val != 0)
            {
                auto maxBits = (uint8_t)(8 * size());
                for (uint8_t i = 0; i < maxBits; ++i)
                {
                    if (val & GetBitMask(i))
                    {
                        if (m_flagNames[i].empty())
                        {
                            TRACE_ERROR("Unable to find flag name for bit {}", i);
                            return false;
                        }

                        stream.beginArrayElement();
                        stream.writeValue(m_flagNames[i].c_str());
                        stream.endArrayElement();
                    }
                }
            }

            return true;
        }

        bool BitfieldType::readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            uint64_t val = 0;

            while (stream.beginArrayElement())
            {
                StringView<char> flagName;
                if (!stream.readValue(flagName))
                {
                    TRACE_ERROR("Expected bit filed flag to be a string");
                    return false;
                }

                uint64_t localVal = 0;
                if (!findFlag(StringID(flagName), localVal))
                {
                    TRACE_ERROR("Unable to assign value to flag '{}'", flagName);
                    return false;
                }

                val |= localVal;

                stream.endArrayElement();
            }

            writeUint64(data, val);
            return true;
        }
        
        void BitfieldType::readUint64(const void* data, uint64_t& outValue) const
        {
            switch (size())
            {
                case 1: outValue = (uint64_t) *(const uint8_t*)data; break;
                case 2: outValue = (uint64_t) *(const uint16_t*)data; break;
                case 4: outValue = (uint64_t) *(const uint32_t*)data; break;
                case 8: outValue = (uint64_t) *(const uint64_t*)data; break;
            }
        }

        void BitfieldType::writeUint64(void* data, uint64_t value) const
        {
            switch (size())
            {
                case 1: *(uint8_t*)data = (uint8_t)value; break;
                case 2: *(uint16_t*)data = (uint16_t)value; break;
                case 4: *(uint32_t*)data = (uint32_t)value; break;
                case 8: *(uint64_t*)data = value; break;
            }
        }

        //--

        bool BitfieldType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                // report bit fields as "structure" like
                outInfo.flags |= DataViewInfoFlagBit::LikeStruct;

                // report bitfield as list of "boolean" members
                if (outInfo.requestFlags.test(DataViewRequestFlagBit::MemberList))
                {
                    for (uint32_t i=0; i<MAX_BITS; ++i)
                    {
                        if (m_flagNames[i])
                        {
                            auto& memberInfo = outInfo.members.emplaceBack();
                            memberInfo.name = m_flagNames[i];
                        }
                    }
                }

                return IType::describeDataView(viewPath, viewData, outInfo);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                uint32_t bitIndex = 0;
                if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                {
                    if (viewPath.empty())
                    {
                        static Type boolType = RTTI::GetInstance().findType("bool"_id);

                        outInfo.flags = DataViewInfoFlagBit::LikeValue;
                        outInfo.dataPtr = nullptr;
                        outInfo.dataType = boolType;
                        return true;
                    }
                }
            }

            return false;
        }

        bool BitfieldType::readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                // allow to read/write whole value
                return IType::readDataView(context, rootView, rootViewPath, viewPath, viewData, targetData, targetType);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                uint32_t bitIndex = 0;
                if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                {
                    if (viewPath.empty())
                    {
                        if (targetType == DataViewBaseValue::GetStaticClass())
                            return IType::readDataView(context, rootView, TempString("{}.{}", rootViewPath, propertyName), viewPath, viewData, targetData, targetType);

                        static Type boolType = RTTI::GetInstance().findType("bool"_id);

                        uint64_t bitMask = 0;
                        readUint64(viewData, bitMask);

                        const bool bitValue = 0 != (GetBitMask(bitIndex) & bitMask);
                        return ConvertData(&bitValue, boolType, targetData, targetType);
                    }
                }
            }

            return false;
        }

        bool BitfieldType::writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                // allow to read/write whole value
                return IType::writeDataView(context, rootView, rootViewPath, viewPath, viewData, sourceData, sourceType);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                uint32_t bitIndex = 0;
                if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                {
                    if (viewPath.empty())
                    {
                        if (sourceType == DataViewCommand::GetStaticClass())
                            return IType::writeDataView(context, rootView, TempString("{}.{}", rootViewPath, propertyName), viewPath, viewData, sourceData, sourceType);

                        static Type boolType = RTTI::GetInstance().findType("bool"_id);

                        bool bitFlag = false;
                        if (!ConvertData(sourceData, sourceType, &bitFlag, boolType))
                            return false;

                        uint64_t bitMask = 0;
                        readUint64(viewData, bitMask);

                        if (bitFlag)
                            bitMask |= GetBitMask(bitIndex);
                        else
                            bitMask &= ~GetBitMask(bitIndex);

                        writeUint64(viewData, bitMask);
                        return true;
                    }
                }
            }

            return false;
        }

        //--

        void BitfieldType::releaseTypeReferences()
        {
            IType::releaseTypeReferences();
            m_flagBitIndices.clear();
        }

    } // rtti
} // base