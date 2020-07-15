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
        namespace prv
        {

            /// array specification for the native array type
            class NativeArrayType : public IArrayType
            {
            public:
                NativeArrayType(Type innerType, uint32_t size);
                virtual ~NativeArrayType();

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
                virtual bool resizeArrayElements(void* data, uint32_t count) const override final;
                virtual bool removeArrayElement(const void* data, uint32_t index) const override final;
                virtual bool createArrayElement(void* data, uint32_t index) const override final;
                virtual const void* arrayElementData(const void* data, uint32_t index) const override final;
                virtual void* arrayElementData(void* data, uint32_t index) const override final;

                //---

                static const char* TypePrefix;
                static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);

            private:
                uint32_t m_elementCount;
            };

        } // prv
    } // rtti
} // base

