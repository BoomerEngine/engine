/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

// capture interface (triggerable RenderDoc integration)
class GPU_API_COMMON_API IFrameCapture : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_API_RUNTIME);

public:
	IFrameCapture();
	virtual ~IFrameCapture();

	//--

	static UniquePtr<IFrameCapture> ConditionalStartCapture(CommandBuffer* masterCommandBuffer);

	//--
};

//---

END_BOOMER_NAMESPACE_EX(gpu)
