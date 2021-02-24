/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

// capture interface (triggerable RenderDoc integration)
class RENDERING_API_COMMON_API IFrameCapture : public base::NoCopy
{
	RTTI_DECLARE_POOL(POOL_API_RUNTIME);

public:
	IFrameCapture();
	virtual ~IFrameCapture();

	//--

	static base::UniquePtr<IFrameCapture> ConditionalStartCapture(GPUCommandBuffer* masterCommandBuffer);

	//--
};

//---

END_BOOMER_NAMESPACE(rendering)