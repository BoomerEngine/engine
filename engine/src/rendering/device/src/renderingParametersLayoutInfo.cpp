/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingParametersLayoutInfo.h"
#include "renderingConstantsView.h"
#include "renderingImageView.h"
#include "renderingBufferView.h"

#include "base/containers/include/stringBuilder.h"

namespace rendering
{

    //--

    ParametersLayoutInfo::ParametersLayoutInfo()
        : m_hash(0)
        , m_memorySize(0)
    {}

    ParametersLayoutInfo::ParametersLayoutInfo(const ParametersLayoutInfo& other, ParametersLayoutID id)
        : m_hash(other.m_hash)
        , m_memorySize(other.m_memorySize)
        , m_entries(other.m_entries)
        , m_id(id)
    {}

    bool ParametersLayoutInfo::validateMemory(const void* memoryPtr, uint32_t memorySize) const
    {
        DEBUG_CHECK_EX(memorySize >= m_memorySize, "Amount of provided memory does not match the amount required by this descriptor layout");
        if (memorySize < m_memorySize)
            return false;

        auto readPtr  = (const uint8_t*)memoryPtr;
        auto endPtr  = readPtr + m_memorySize;
        for (auto type : m_entries)
        {
            DEBUG_CHECK_EX(readPtr < endPtr, "Memory size of data does not match descriptor layout");
            if (readPtr >= endPtr)
                return false;

            DEBUG_CHECK_EX(*readPtr == (uint8_t)type, "Memory layout of data does not match descriptor layout");
            if (*readPtr != (uint8_t)type)
                return false;

            switch (type)
            {
                case ObjectViewType::Constants:
                {
                    DEBUG_CHECK_EX(readPtr + sizeof(ConstantsView) <= endPtr, "Memory size of data does not match descriptor layout");
                    if (readPtr + sizeof(ConstantsView) > endPtr)
                        return false;

                    auto view  = (const ConstantsView*)readPtr;
                    readPtr += sizeof(ConstantsView);
                    break;
                }

                case ObjectViewType::Buffer:
                {
                    DEBUG_CHECK_EX(readPtr + sizeof(BufferView) <= endPtr, "Memory size of data does not match descriptor layout");
                    if (readPtr + sizeof(BufferView) > endPtr)
                        return false;

                    auto view  = (const BufferView*)readPtr;
                    readPtr += sizeof(BufferView);
                    break;
                }

                case ObjectViewType::Image:
                {
                    DEBUG_CHECK_EX(readPtr + sizeof(ImageView) <= endPtr, "Memory size of data does not match descriptor layout");
                    if (readPtr + sizeof(ImageView) > endPtr)
                        return false;

                    auto view  = (const ImageView*)readPtr;
                    readPtr += sizeof(ImageView);
                    break;
                }
            }
        }

        return (readPtr == endPtr);
    }

    bool ParametersLayoutInfo::validateBindings(const void* memoryPtr) const
    {
        auto readPtr = (const uint8_t*)memoryPtr;
        for (auto type : m_entries)
        {
            DEBUG_CHECK_EX(*readPtr == (uint8_t)type, "Memory layout of data does not match descriptor layout");
            if (*readPtr != (uint8_t)type)
                return false;

            switch (type)
            {
                case ObjectViewType::Constants:
                {
                    auto& view = *(const ConstantsView*)readPtr;
                    DEBUG_CHECK_EX(view, "Constant view is biding is missing in descriptor");
                    readPtr += sizeof(ConstantsView);
                    break;
                }

                case ObjectViewType::Buffer:
                {
                    auto& view = *(const BufferView*)readPtr;
                    DEBUG_CHECK_EX(view, "Buffer biding is missing in descriptor");
                    readPtr += sizeof(BufferView);
                    break;
                }

                case ObjectViewType::Image:
                {
                    auto& view = *(const ImageView*)readPtr;
                    DEBUG_CHECK_EX(view, "Image biding is missing in descriptor");
                    readPtr += sizeof(ImageView);
                    break;
                }
            }
        }

        return true;
    }

    void ParametersLayoutInfo::print(base::IFormatStream& str) const
    {
        for (auto type : m_entries)
        {
            switch (type)
            {
                case ObjectViewType::Constants:
                    str.append("C");
                    break;

                case ObjectViewType::Buffer:
                    str.append("B");
                    break;

                case ObjectViewType::Image:
                    str.append("I");
                    break;

                default:
                    FATAL_ERROR(base::TempString("Invalid type of the object view {} in the parameter layout", (uint8_t) type));
                    break;
            }
        }
    }

    bool ParametersLayoutInfo::operator==(const ParametersLayoutInfo& other) const
    {
        if (m_hash != other.m_hash)
            return false;

        //ASSERT(m_entries == other.m_entries);
        return true;
    }

    //--


} // rendering