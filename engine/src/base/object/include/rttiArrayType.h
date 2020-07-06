/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"

namespace base
{
    namespace rtti
    {
        /// Meta type of the array type
        enum class ArrayMetaType
        {
            Dynamic,    // a dynamic array (Array)
            Native,     // classic array T[N]
        };

        /// Base type in the RTTI system
        class BASE_OBJECT_API IArrayType : public IType
        {
        public:
            IArrayType(StringID name, Type innerType);
            virtual ~IArrayType();

            /// get the inner array type
            INLINE Type innerType() const { return m_innertType; }

            /// get meta type for the handle type :)
            virtual ArrayMetaType arrayMetaType() const = 0;

            /// get current size of the array
            virtual uint32_t arraySize(const void* data) const = 0;

            /// get current array capacity
            virtual uint32_t arrayCapacity(const void* data) const = 0;

			/// get maximum array capacity, limited for static types
			virtual uint32_t maxArrayCapacity(const void* data) const = 0;

            /// can the array be resized ?
            virtual bool canArrayBeResized() const = 0;

            /// clear array (remove all elements)
            virtual bool clearArrayElements(void* data) const = 0;

            /// remove element from array at given index, returns false if failed
            virtual bool removeArrayElement(const void* data, uint32_t index) const = 0;

            /// add and construct new array element at given index, returns false if failed
            virtual bool createArrayElement(void* data, uint32_t index) const = 0;

            /// get pointer to element data (const version)
            virtual const void* arrayElementData(const void* data, uint32_t index) const = 0;

            /// get pointer to element data (non const version)
            virtual void* arrayElementData(void* data, uint32_t index) const = 0;

            //----

            virtual bool compare(const void* data1, const void* data2) const override final;
            virtual void copy(void* dest, const void* src) const override final;
            virtual void calcCRC64(CRC64& crc, const void* data) const override final;

            virtual bool writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const override final;
            virtual bool readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const override final;

            virtual bool writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const override final;
            virtual bool readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const override final;

            virtual DataViewResult describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const override;
            virtual DataViewResult readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const override final;
            virtual DataViewResult writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const override ;

        private:
            Type m_innertType;
        };

    } // rtti
} // base