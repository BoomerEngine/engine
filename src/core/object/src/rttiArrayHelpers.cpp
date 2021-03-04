/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\arrays #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiArrayHelpers.h"

BEGIN_BOOMER_NAMESPACE()

//--

void ArrayGeneric::destroy(BaseArray* array, Type elementType) const
{
    const auto currentCapacity = array->capacity();

    clear(array, elementType);

    const auto currentBufferSize = currentCapacity * elementType->size();
    array->changeCapacity(0, currentBufferSize, 0, elementType->alignment(), "");
}

void ArrayGeneric::clear(BaseArray* array, Type elementType) const
{
    if (elementType->traits().requiresDestructor)
    {
        auto stride = elementType->size();

        auto writePtr  = (uint8_t*)array->data();
        auto endPtr  = writePtr + array->size() * stride;

        while (writePtr < endPtr)
        {
            elementType->destruct(writePtr);
            writePtr += stride;
        }
    }

    array->changeSize(0);
}

void ArrayGeneric::reserve(BaseArray* array, Type elementType, uint32_t minimalSize) const
{
    if (array->capacity() < minimalSize)
    {
        auto stride = elementType->size();
                
        const auto minimalBufferSize = minimalSize * stride;
        const auto currentBufferSize = array->capacity() * stride;
        auto newBufferSize = currentBufferSize;

        while (newBufferSize < minimalBufferSize)
            newBufferSize = BaseArray::CalcNextBufferSize(newBufferSize);

        const auto newCapacity = newBufferSize / stride;

        array->changeCapacity(newCapacity, currentBufferSize, newBufferSize, elementType->alignment(), elementType->name().c_str());
    }
}

void ArrayGeneric::resize(BaseArray* array, Type elementType, uint32_t requestedSize) const
{
    reserve(array, elementType, requestedSize);

    if (requestedSize > array->size())
    {
        if (elementType->traits().requiresConstructor)
        {
            auto stride = elementType->size();

            auto writePtr = (uint8_t *) array->data() + (array->size() * stride);
            auto endPtr = (uint8_t *) array->data() + (requestedSize * stride);

            if (elementType->traits().initializedFromZeroMem)
            {
                memzero(writePtr, endPtr - writePtr);
            }
            else
            {
                while (writePtr < endPtr)
                {
                    elementType->construct(writePtr);
                    writePtr += stride;
                }
            }
        }
    }
    else if (requestedSize < array->size())
    {
        if (elementType->traits().requiresDestructor)
        {
            auto stride = elementType->size();

            auto writePtr = (uint8_t *) array->data() + (requestedSize * stride);
            auto endPtr = writePtr + (array->size() * stride);

            while (writePtr < endPtr)
            {
                elementType->destruct(writePtr);
                writePtr += stride;
            }
        }
    }

    array->changeSize(requestedSize);
}

void ArrayGeneric::eraseRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const
{
    ASSERT(index + count <= array->size());

    auto stride = elementType->size();

    if (elementType->traits().requiresDestructor)
    {
		auto writePtr  = (uint8_t*)array->data() + (index * stride);
		auto writeEndPtr  = (uint8_t*)writePtr + (count * stride);
		while (writePtr < writeEndPtr)
        {
            elementType->destruct(writePtr);
            writePtr += stride;
        }
    }

	{
		auto writePtr  = (uint8_t*)array->data() + (index * stride);
		memmove(writePtr, writePtr + (count * stride), (array->size() - index - count) * stride);
	}

    array->changeSize(array->size() - count);
}

void ArrayGeneric::insertRange(BaseArray* array, Type elementType, uint32_t index, uint32_t count) const
{
    ASSERT(index <= array->size());

    reserve(array, elementType, array->size() + count);

    auto stride = elementType->size();

	{
		auto writePtr  = (uint8_t*)array->data() + (index * stride);
		memmove(writePtr + (count * stride), writePtr, (array->size() - index) * stride);
	}

    if (elementType->traits().requiresConstructor)
    {
		auto writePtr  = (uint8_t*)array->data() + (index * stride);
		auto writeEndPtr  = (uint8_t*)writePtr + (count * stride);
        if (elementType->traits().initializedFromZeroMem)
        {
            memzero(writePtr, writeEndPtr - writePtr);
        }
        else
        {
            while (writePtr < writeEndPtr)
            {
                elementType->construct(writePtr);
                writePtr += stride;
            }
        }
    }

    array->changeSize(array->size() + count);
}

//--

static ArrayGeneric GArrayGeneric;
static ArrayPOD<1> GArrayPOD1;
static ArrayPOD<2> GArrayPOD2;
static ArrayPOD<4> GArrayPOD4;
static ArrayPOD<8> GArrayPOD8;
static ArrayPOD<12> GArrayPOD12;
static ArrayPOD<16> GArrayPOD16;
static ArrayPOD<32> GArrayPOD32;
static ArrayPOD<64> GArrayPOD64;

const IArrayHelper* IArrayHelper::GetHelperForType(Type elementType)
{
    auto& traits = elementType->traits();
    if (!traits.requiresDestructor && !traits.requiresConstructor && traits.simpleCopyCompare)
    {
        auto expectedAlignment = elementType->size();
        if (expectedAlignment <= 16)
            expectedAlignment = 4;
        else if (expectedAlignment >= 16)
            expectedAlignment = 16;

        if (expectedAlignment == elementType->alignment())
        {
            switch (elementType->size())
            {
            case 1: return &GArrayPOD1;
            case 2: return &GArrayPOD2;
            case 4: return &GArrayPOD4;
            case 8: return &GArrayPOD8;
            case 12: return &GArrayPOD12;
            case 16: return &GArrayPOD16;
            case 32: return &GArrayPOD32;
            case 64: return &GArrayPOD64;
            }
        }
    }

    return &GArrayGeneric;
}

//--

END_BOOMER_NAMESPACE()
