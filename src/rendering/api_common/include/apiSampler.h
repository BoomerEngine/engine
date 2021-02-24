/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

#include "rendering/device/include/renderingSamplerState.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

/// general sampler object
class RENDERING_API_COMMON_API IBaseSampler : public IBaseObject
{
public:
	IBaseSampler(IBaseThread* owner, const SamplerState& setup);
	virtual ~IBaseSampler();

	static const auto STATIC_TYPE = ObjectType::Sampler;

	//--

	INLINE const SamplerState& state() const { return m_state; }

	//--

private:
	SamplerState m_state;
};

//---

END_BOOMER_NAMESPACE(rendering::api)