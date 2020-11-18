/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\objects #]
***/

#include "build.h"
#include "renderingDeviceApi.h"
#include "renderingObject.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{

    //---

    void ObjectID::print(base::IFormatStream& f) const
    {
        switch (data.fields.type)
        {
            case TYPE_NULL:
                f.append("EMPTY");
                break;

            case TYPE_STATIC:
                f.appendf("STATIC {}@{}", generation(), index());
                break;

            case TYPE_PREDEFINED:
                f.appendf("PREDEFINED {}", index());
                break;

            case TYPE_TRANSIENT:
                f.appendf("TRANSIENT {}", index());
                break;
        }           
    }

    static std::atomic<uint32_t> ObjectIDTransientIdAllocator(1);

    ObjectID ObjectID::AllocTransientID()
    {
        ObjectID ret;
        ret.data.fields.type = TYPE_TRANSIENT;
        ret.data.fields.index = ++ObjectIDTransientIdAllocator;
        return ret;
    }

    ObjectID ObjectID::CreateStaticID(uint32_t internalIndex, uint32_t internalGeneration)
    {
        DEBUG_CHECK(internalGeneration != 0);

        ObjectID ret;
        ret.data.fields.type = TYPE_STATIC;
        ret.data.fields.index = internalIndex;
        ret.data.fields.generation = internalGeneration;
        return ret;
    }

    ObjectID ObjectID::CreatePredefinedID(uint8_t predefinedIndex)
    {
        if (!predefinedIndex)
            return EMPTY();

        ObjectID ret;
        ret.data.fields.type = TYPE_PREDEFINED;
        ret.data.fields.index = predefinedIndex;
        return ret;
    }

    static ObjectID TheEmptyObject;

    const ObjectID& ObjectID::EMPTY()
    {
        return TheEmptyObject;
    }

    //--

    ObjectID ObjectID::DefaultPointSampler(bool clamp)
    {
        return ObjectID::CreatePredefinedID(clamp ? ID_SamplerClampPoint : ID_SamplerWrapPoint);
    }

    ObjectID ObjectID::DefaultBilinearSampler(bool clamp)
    {
        return ObjectID::CreatePredefinedID(clamp ? ID_SamplerClampBiLinear : ID_SamplerWrapBiLinear);
    }

    ObjectID ObjectID::DefaultTrilinearSampler(bool clamp)
    {
        return ObjectID::CreatePredefinedID(clamp ? ID_SamplerClampTriLinear : ID_SamplerWrapTriLinear);
    }

    ObjectID ObjectID::DefaultDepthPointSampler()
    {
        return ObjectID::CreatePredefinedID(ID_SamplerPointDepthLE);
    }

    ObjectID ObjectID::DefaultDepthBilinearSampler()
    {
        return ObjectID::CreatePredefinedID(ID_SamplerBiLinearDepthLE);
    }

    //--

} // rendering