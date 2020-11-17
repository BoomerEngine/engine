/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"
#include "base/containers/include/array.h"
#include "base/containers/include/stringID.h"

namespace base
{
    namespace rtti
    {

        /// Collection of named flags
        class BASE_OBJECT_API BitfieldType : public IType
        {
        public:
            BitfieldType(StringID name, uint32_t size, uint64_t nativeHash);

            /// add an option to the enum
            void add(StringID name, uint64_t bitMask);

            /// find a bit maks for a flag with given name
            /// returns false if matching option was not found
            bool findFlag(StringID name, uint64_t& outBitMaks) const;

            /// find name for given bit mask
            /// returns false if matching option was not found
            bool findName(uint64_t bitMask, StringID& outName) const;

            //--
  
            /// get all options
            void options(Array<StringID>& outInfos) const;

            //---

            // rtti::IType interface
            virtual void construct(void *mem) const override final {};
            virtual void destruct(void *mem) const override final {};
            
            virtual bool compare(const void* data1, const void* data2) const override final;
            virtual void copy(void* dest, const void* src) const override final;

            virtual void writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const override final;
            virtual void readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const override final;

            virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
            virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

            virtual DataViewResult describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const override final;
            virtual DataViewResult readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const override final;
            virtual DataViewResult writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const override final;

        private:
            static const uint32_t MAX_BITS = 64;
            
            StringID m_flagNames[64];
            HashMap<StringID, uint32_t> m_flagBitIndices;

            void readUint64(const void* data, uint64_t & outValue) const;
            void writeUint64(void* data, uint64_t value) const;

            static uint8_t GetBitIndex(uint64_t bitMask);
            static uint64_t GetBitMask(uint8_t bitIndex);

            virtual void releaseTypeReferences() override;
        };

    } // rtti
} // base