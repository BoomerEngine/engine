/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#include "build.h"
#include "renderingObjectID.h"

#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

RTTI_BEGIN_TYPE_ENUM(ResourceLayout)
    RTTI_ENUM_OPTION(Common);
    RTTI_ENUM_OPTION(ConstantBuffer);
    RTTI_ENUM_OPTION(VertexBuffer);
    RTTI_ENUM_OPTION(IndexBuffer);
    RTTI_ENUM_OPTION(RenderTarget);
    RTTI_ENUM_OPTION(UAV);
    RTTI_ENUM_OPTION(DepthWrite);
    RTTI_ENUM_OPTION(DepthRead);
    RTTI_ENUM_OPTION(ShaderResource);
    RTTI_ENUM_OPTION(IndirectArgument);
    RTTI_ENUM_OPTION(CopyDest);
    RTTI_ENUM_OPTION(CopySource);
    RTTI_ENUM_OPTION(ResolveDest);
    RTTI_ENUM_OPTION(ResolveSource);
    RTTI_ENUM_OPTION(RayTracingAcceleration);
    RTTI_ENUM_OPTION(ShadingRateSource);
    RTTI_ENUM_OPTION(Present);        
RTTI_END_TYPE();


//---

void ObjectID::print(base::IFormatStream& f) const
{
	if (empty())
	{
		f.append("EMPTY");
	}
	else
	{
#ifdef BUILD_RELEASE
		f.appendf("ID {}, GEN {}", m_index, m_generation);
#else
		f.appendf("ID {}, GEN {} @ {}", m_index, m_generation, m_ptr.rawPtr);
#endif
	}
}
                
static ObjectID TheEmptyObject;

const ObjectID& ObjectID::EMPTY()
{
    return TheEmptyObject;
}

//--

/*ObjectID ObjectID::DefaultPointSampler(bool clamp)
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
}*/

//--

END_BOOMER_NAMESPACE(rendering)