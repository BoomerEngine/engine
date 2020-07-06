/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\arrays #]
***/

#include "rttiArrayType.h"

namespace base
{
    namespace rtti
    {
        class IArrayHelper;

        namespace prv
        {

            /// array specification for the dynamic array type
            class DynamicArrayType : public IArrayType
            {
            public:
                DynamicArrayType(Type innerType);
                virtual ~DynamicArrayType();

                virtual void construct(void *object) const override final;
                virtual void destruct(void *object) const override final;
                virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override final;
                virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags = 0) const override final;

                virtual ArrayMetaType arrayMetaType() const override final;
                virtual uint32_t arraySize(const void* data) const override final;
                virtual uint32_t arrayCapacity(const void* data) const override final;
				virtual uint32_t maxArrayCapacity(const void* data) const override final;
                virtual bool canArrayBeResized() const override final;
                virtual bool clearArrayElements(void* data) const override final;
                virtual bool removeArrayElement(const void* data, uint32_t index) const override final;
                virtual bool createArrayElement(void* data, uint32_t index) const override final;
                virtual const void* arrayElementData(const void* data, uint32_t index) const override final;
                virtual void* arrayElementData(void* data, uint32_t index) const override final;

                virtual DataViewResult describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const override final;
                virtual DataViewResult writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const override final;

                //---

                static const char* TypePrefix;
                static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);

            private:
                const IArrayHelper* m_helper;

                virtual void cacheTypeData() override final;
            };

        } // prv
    } // rtti
} // base

