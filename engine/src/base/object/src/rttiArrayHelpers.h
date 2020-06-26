/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\arrays #]
***/

#pragma once

#include "base/containers/include/baseArray.h"

namespace base
{
    namespace rtti
    {

        //--

        /// array helper interface
        class BASE_OBJECT_API IArrayHelper : public NoCopy
        {
        public:
            virtual void destroy(BaseArray* array, Type elementType) const = 0;
            virtual void clear(BaseArray* array, Type elementType) const = 0;
            virtual void reserve(BaseArray* array, Type elementType, uint32_t minimalSize) const = 0;
            virtual void resize(BaseArray* array, Type elementType, uint32_t requestedSize) const = 0;
            virtual void eraseRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const = 0;
            virtual void insertRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const = 0;

            //--

            static const IArrayHelper* GetHelperForType(Type type);
        };

        //--

        /// generic array helper, handles all types
        class BASE_OBJECT_API ArrayGeneric : public IArrayHelper
        {
        public:
            virtual void destroy(BaseArray* array, Type elementType) const override final;
            virtual void clear(BaseArray* array, Type elementType) const override final;
            virtual void reserve(BaseArray* array, Type elementType, uint32_t minimalSize) const override final;
            virtual void resize(BaseArray* array, Type elementType, uint32_t requestedSize) const override final;
            virtual void eraseRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const override final;
            virtual void insertRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const override final;
        };

        //--

        /// array for POD types
        template< uint32_t N >
        class BASE_OBJECT_API ArrayPOD : public IArrayHelper
        {
        public:
            uint8_t m_alignment;

            ArrayPOD()
            {
                if (N <= 4)
                    m_alignment = N;
                else if (N <= 16)
                    m_alignment = 4;
                else if (N == 16)
                    m_alignment = 16;
            }

            virtual void destroy(BaseArray* array, Type elementType) const override final
            {
                array->changeSize(0);
                array->changeCapacity(POOL_CONTAINERS, 0, N, m_alignment, "PodArray");
            }

            virtual void clear(BaseArray* array, Type elementType) const override final
            {
                array->changeSize(0);
            }

            virtual void reserve(BaseArray* array, Type elementType, uint32_t minimalSize) const override final
            {
                if (array->capacity() < minimalSize)
                {
                    auto newCapacity = BaseArray::CalcNextCapacity(array->capacity(), N);
                    while (newCapacity < minimalSize)
                        newCapacity = BaseArray::CalcNextCapacity(newCapacity, N);

                    array->changeCapacity(POOL_CONTAINERS, newCapacity, N, m_alignment, "PodArray");
                }
            }

            virtual void resize(BaseArray* array, Type elementType, uint32_t requestedSize) const override final
            {
                reserve(array, elementType, requestedSize);
                array->changeSize(requestedSize);
            }

            virtual void eraseRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const override final
            {
                ASSERT(index + count <= array->size());

				auto writePtr  = (uint8_t*)array->data() + (index * N);
				memmove(writePtr, writePtr + (count * N), (array->size() - index - count) * N);

                array->changeSize(array->size() - count);
            }

            virtual void insertRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const override final
            {
                ASSERT(index <= array->size());

                reserve(array, elementType, array->size() + count);

				auto writePtr  = (uint8_t*)array->data() + (index * N);
				memmove(writePtr + (count * N), writePtr, (array->size() - index) * N);

                array->changeSize(array->size() + count);
            }
        };

        //--

    } // rtti
} // base

