/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiBitfieldType.h"

#include "streamOpcodeWriter.h"
#include "streamOpcodeReader.h"
#include "rttiDataView.h"
#include "dataView.h"
#include "base/containers/include/inplaceArray.h"

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
        
        void BitfieldType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            uint64_t val = 0;
            readUint64(data, val);

            // gather flags to write
            InplaceArray<StringID, 64> flagNames;
            auto maxBits = (uint8_t)(8 * size());
            for (uint8_t i = 0; i < maxBits; ++i)
            {
                if (val & GetBitMask(i))
                {
                    if (m_flagNames[i].empty())
                    {
                        TRACE_WARNING("Missing flag name for flag {} in {}, the bit value will be lost", i, name());
                        continue;
                    }

                    flagNames.pushBack(m_flagNames[i]);
                }
            }

            // write as array of names
            file.beginArray(flagNames.size());
            for (const auto& name : flagNames)
                file.writeStringID(name);
            file.endArray();
        }

        void BitfieldType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            uint64_t val = 0;

            uint32_t flagCount = 0;
            file.enterArray(flagCount);

            for (uint32_t i = 0; i < flagCount; ++i)
            {
                auto flagName = file.readStringID();

                uint64_t flagBitMask = 0;
                if (!findFlag(flagName, flagBitMask))
                {
                    TRACE_WARNING("Failed to find flag value for flag '{}' in bitfield '{}'", flagName, name());
                    continue;
                }

                val |= flagBitMask;
            }

            writeUint64(data, val);
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

        DataViewResult BitfieldType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            const auto orgViewPath = viewPath;

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

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                if (viewPath.empty())
                {
                    uint32_t bitIndex = 0;
                    if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                    {
                        static Type boolType = RTTI::GetInstance().findType("bool"_id);

                        outInfo.flags = DataViewInfoFlagBit::LikeValue;
                        outInfo.dataPtr = nullptr;
                        outInfo.dataType = boolType;
                        return DataViewResultCode::OK;
                    }
                    else if (propertyName == "__rawSize")
                    {
                        // TODO: report bitfield data size (uint8, 16, 32, 64, etc)
                    }
                    else if (propertyName == "__rawValue")
                    {
                        // TODO: report bitfield raw value
                    }
                }
            }

            return IType::describeDataView(orgViewPath, viewData, outInfo);
        }

        DataViewResult BitfieldType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            const auto orgViewPath = viewPath;

            if (!viewPath.empty())
            {
                StringView<char> propertyName;
                if (ParsePropertyName(viewPath, propertyName))
                {
                    uint32_t bitIndex = 0;
                    if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                    {
                        uint64_t bitMask = 0;
                        readUint64(viewData, bitMask);

                        const bool bitValue = 0 != (GetBitMask(bitIndex) & bitMask);

                        static Type boolType = RTTI::GetInstance().findType("bool"_id);
                        return boolType->readDataView(viewPath, &bitValue, targetData, targetType);
                    }
                    else if (propertyName == "__rawSize")
                    {
                        uint32_t value = size();

                        static Type uint32Type = RTTI::GetInstance().findType("uint32_t"_id);
                        return uint32Type->readDataView(viewPath, &value, targetData, targetType);
                    }
                    else if (propertyName == "__rawValue")
                    {
                        uint64_t bitMask = 0;
                        readUint64(viewData, bitMask);

                        static Type uint64Type = RTTI::GetInstance().findType("uint64_t"_id);
                        return uint64Type->readDataView(viewPath, &bitMask, targetData, targetType);
                    }
                }
            }

            return IType::readDataView(orgViewPath, viewData, targetData, targetType);
        }

        DataViewResult BitfieldType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            const auto orgViewPath = viewPath;

            if (!viewPath.empty())
            {
                StringView<char> propertyName;
                if (ParsePropertyName(viewPath, propertyName))
                {
                    uint32_t bitIndex = 0;
                    if (m_flagBitIndices.find(StringID::Find(propertyName), bitIndex))
                    {
                        static Type boolType = RTTI::GetInstance().findType("bool"_id);

                        bool bitFlag = false;
                        if (auto ret = HasError(boolType->writeDataView(viewPath, &bitFlag, sourceData, sourceType)))
                            return ret;

                        uint64_t bitMask = 0;
                        readUint64(viewData, bitMask);

                        if (bitFlag)
                            bitMask |= GetBitMask(bitIndex);
                        else
                            bitMask &= ~GetBitMask(bitIndex);

                        writeUint64(viewData, bitMask);
                        return DataViewResultCode::OK;
                    }
                    else if (propertyName == "__rawSize")
                    {
                        return DataViewResultCode::ErrorReadOnly;
                    }
                    else if (propertyName == "__rawValue")
                    {
                        uint64_t rawValue = 0;

                        static Type uint64Type = RTTI::GetInstance().findType("uint64_t"_id);
                        if (auto err = HasError(uint64Type->writeDataView(viewPath, &rawValue, sourceData, sourceType)))
                            return err;

                        writeUint64(viewData, rawValue);
                        return DataViewResultCode::OK;
                    }
                }
            }

            return IType::writeDataView(orgViewPath, viewData, sourceData, sourceType);
        }

        //--

        void BitfieldType::releaseTypeReferences()
        {
            IType::releaseTypeReferences();
            m_flagBitIndices.clear();
        }

    } // rtti
} // base