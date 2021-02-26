/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/renderingOutput.h"

#include "apiObject.h"
#include "apiSwapchain.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

class ObjectRegistryProxy;

//--

// "virtual" render target view of the swapchain 
class GPU_API_COMMON_API OutputRenderTarget : public IBaseObject
{
public:
	OutputRenderTarget(IBaseThread* owner, Output* output, bool depth);

	INLINE Output* output() const { return m_output; }
	INLINE bool depth() const { return m_depth; }

private:
	Output* m_output = nullptr;
	bool m_depth = false;
};

//--

// an output object - higher level wrapper for the swapchain
class GPU_API_COMMON_API Output : public IBaseObject
{
public:
	Output(IBaseThread* owner, IBaseSwapchain* swapchain);
	virtual ~Output();

	static const auto STATIC_TYPE = ObjectType::Output;

	//---

	// prepare output for rendering - called from any thread, should return current state
	bool prepare_ClientApi(IDeviceObject* owningObject, RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, Point& outViewport);

	// disconnect window from client side callbacks
	void disconnect_ClientApi();

	//--

	// acquire surface from owned swapchain
	bool acquire();

	// present surface on owned swapchain (swap or discard)
	void present(bool swap = true);

	//--

protected:
	IBaseSwapchain* m_swapchain = nullptr;

	OutputRenderTarget* m_colorTarget = nullptr;
	OutputRenderTarget* m_depthTarget = nullptr;

	//--

	virtual void disconnectFromClient() override final;
};

//---

// engine side proxy for device output
class GPU_API_COMMON_API OutputObjectProxy : public gpu::IOutputObject
{
public:
	OutputObjectProxy(ObjectID id, IDeviceObjectHandler* impl, bool flipped, INativeWindowInterface* window);

	virtual bool prepare(RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, Point& outViewport) override final;
	virtual void disconnect() override final;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
