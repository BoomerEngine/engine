/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiCapture.h"

#include "gpu/device/include/renderingCommandBuffer.h"
#include "gpu/device/include/renderingCommands.h"
#include "gpu/device/include/renderingDeviceApi.h"

#ifdef PLATFORM_WINAPI
#include <Windows.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#endif

#ifndef BUILD_FINAL
	#define USE_RENDER_DOC
#endif

#ifdef USE_RENDER_DOC
	#include "renderdoc_api.h"
#endif

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

IFrameCapture::IFrameCapture()
{}

IFrameCapture::~IFrameCapture()
{}

//--

#ifdef USE_RENDER_DOC

class RenderDocCapture : public IFrameCapture
{
    RTTI_DECLARE_POOL(POOL_API_RUNTIME)

public:
    RenderDocCapture()
        : m_api(nullptr)
    {
#ifdef PLATFORM_WINAPI
        if (HMODULE mod = (HMODULE)LoadLibraryW(L"renderdoc.dll"))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_api);
            if (ret == 1)
            {
                TRACE_INFO("Attached to RenderDOC");

                if (m_api)
                    m_api->StartFrameCapture(NULL, NULL);
            }
            else
            {
                TRACE_WARNING("Failed to attach to RenderDOC");
            }
        }
        else
        {
            TRACE_WARNING("No RenderDOC to attach to");
        }
#elif defined(PLATFORM_LINUX)
        if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_api);
            if (ret == 1)
            {
                TRACE_INFO("Attached to RenderDOC");

                if (m_api)
                    m_api->StartFrameCapture(NULL, NULL);
            }
            else
            {
                TRACE_WARNING("Failed to attach to RenderDOC");
            }
        }
        else
        {
            TRACE_WARNING("No RenderDOC to attach to");
        }
#endif
    }

    ~RenderDocCapture()
    {
        if (m_api)
        {
            m_api->EndFrameCapture(NULL, NULL);
            TRACE_INFO("Finished RenderDOC capture");
        }
    }

private:
    RENDERDOC_API_1_1_2* m_api;
};

#endif

//---

static bool HasTriggeredCapture(const CommandBuffer* masterCommandBuffer)
{
	auto* cmd = masterCommandBuffer->commands();
	while (cmd)
	{
		if (cmd->op == CommandCode::TriggerCapture)
		{
			return true;
		}
		else if (cmd->op == CommandCode::ChildBuffer)
		{
			auto* op = static_cast<const OpChildBuffer*>(cmd);
			if (HasTriggeredCapture(op->childBuffer))
				return true;
		}

		cmd = GetNextCommand(cmd);
	}

	return false;
}
       
UniquePtr<IFrameCapture> IFrameCapture::ConditionalStartCapture(CommandBuffer* masterCommandBuffer)
{
#ifndef BUILD_RELEASE
#ifdef USE_RENDER_DOC
	if (HasTriggeredCapture(masterCommandBuffer))
		return CreateUniquePtr<RenderDocCapture>();
#endif
#endif

    return nullptr;
}

//---

END_BOOMER_NAMESPACE_EX(gpu::api)
