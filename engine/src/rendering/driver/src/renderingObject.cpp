/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\objects #]
***/

#include "build.h"
#include "renderingDriver.h"
#include "renderingObject.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{

    //---

    ObjectID::ObjectID(const EStatic, void* ptr)
    {
        m_value = (uint64_t)ptr;
        ASSERT_EX((m_value & 7) == 0, "Pointer must be aligned to at least 8 bytes"); // pointer
        m_value |= STATIC_BIT;
    }

    ObjectID::ObjectID(const EPredefined, uint32_t internalID)
    {
        m_value = (internalID << 3) | PREDEFINED_BIT;
    }

    ObjectID::ObjectID(const ETransient, uint32_t internalID)
    {
        m_value = (internalID << 3) | TRANSIENT_BIT;
    }

    void ObjectID::reset()
    {
        m_value = 0;
    }

    void ObjectID::print(base::IFormatStream& f) const
    {
        if (empty())
            f.append("EMPTY");
        else if (isPredefined())
            f.appendf("PREDEFINED: {}", internalIndex());
        else if (isTransient())
            f.appendf("TRANSIENT {}", internalIndex());
        else
            f.appendf("STATIC {}", Hex(internalPointer()));
    }

    static std::atomic<uint32_t> ObjectIDTransientIdAllocator(1);

    ObjectID ObjectID::AllocTransientID()
    {
        return ObjectID(ObjectID::TRANSIENT, ++ObjectIDTransientIdAllocator);
    }

    static ObjectID TheEmptyObject;

    const ObjectID& ObjectID::EMPTY()
    {
        return TheEmptyObject;
    }

    //--

    ObjectID ObjectID::DefaultPointSampler(bool clamp)
    {
        return ObjectID(ObjectID::PREDEFINED, clamp ? ID_SamplerClampPoint : ID_SamplerWrapPoint);
    }

    ObjectID ObjectID::DefaultBilinearSampler(bool clamp)
    {
        return ObjectID(ObjectID::PREDEFINED, clamp ? ID_SamplerClampBiLinear : ID_SamplerWrapBiLinear);
    }

    ObjectID ObjectID::DefaultTrilinearSampler(bool clamp)
    {
        return ObjectID(ObjectID::PREDEFINED, clamp ? ID_SamplerClampTriLinear : ID_SamplerWrapTriLinear);
    }

    ObjectID ObjectID::DefaultDepthPointSampler()
    {
        return ObjectID(ObjectID::PREDEFINED, ID_SamplerPointDepthLE);
    }

    ObjectID ObjectID::DefaultDepthBilinearSampler()
    {
        return ObjectID(ObjectID::PREDEFINED, ID_SamplerBiLinearDepthLE);
    }

    //---

    ObjectDebugInfo::ObjectDebugInfo(base::StringID className, uint32_t deviceMemorySize, uint32_t hostMemorySize)
        : className(className)
        , deviceMemorySize(deviceMemorySize)
        , hostMemorySize(hostMemorySize)
    {}

    //--


} // rendering