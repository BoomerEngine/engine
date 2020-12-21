/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiOutput.h"
#include "apiSwapchain.h"
#include "apiThread.h"
#include "apiObjectRegistry.h"

namespace rendering
{
    namespace api
    {
		//--

		OutputRenderTarget::OutputRenderTarget(IBaseThread* owner, Output* output, bool depth)
			: IBaseObject(owner, ObjectType::OutputRenderTargetView)
			, m_output(output)
			, m_depth(depth)
		{}

		//--

		Output::Output(IBaseThread* owner, IBaseSwapchain* swapchain)
			: IBaseObject(owner, ObjectType::Output)
			, m_swapchain(swapchain)
		{
			m_colorTarget = new OutputRenderTarget(owner, this, false);
			m_depthTarget = new OutputRenderTarget(owner, this, true);
		}

		Output::~Output()
		{
			delete m_colorTarget;
			m_colorTarget = nullptr;

			delete m_depthTarget;
			m_depthTarget = nullptr;

			delete m_swapchain;
			m_swapchain = nullptr;
		}

		bool Output::prepare_ClientApi(IDeviceObject* owningObject, RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport)
		{
			// ask swapchain if it's ready :)
			SwapchainState state;
			if (!m_swapchain->prepare_ClientApi(state))
				return false;

			DEBUG_CHECK_RETURN_EX_V(state.width && state.height, "Invalid swapchain size", false);

			outViewport.x = state.width;
			outViewport.y = state.height;

			if (outColorRT)
			{
				if (state.colorFormat != ImageFormat::UNKNOWN)
				{
					RenderTargetView::Setup setup;
					setup.firstSlice = 0;
					setup.numSlices = 1;
					setup.width = state.width;
					setup.height = state.height;
					setup.samples = state.samples;
					setup.msaa = state.samples > 1;
					setup.mip = 0;
					setup.arrayed = false;
					setup.flipped = true;
					setup.swapchain = true;
					setup.format = state.colorFormat;
					setup.depth = false;

					*outColorRT = base::RefNew<RenderTargetView>(m_colorTarget->handle(), owningObject, owner()->objectRegistry(), setup);
				}
				else
				{
					*outColorRT = nullptr;
				}
			}

			if (outDepthRT)
			{
				if (state.depthFormat != ImageFormat::UNKNOWN)
				{
					RenderTargetView::Setup setup;
					setup.firstSlice = 0;
					setup.numSlices = 1;
					setup.width = state.width;
					setup.height = state.height;
					setup.samples = state.samples;
					setup.msaa = state.samples > 1;
					setup.mip = 0;
					setup.arrayed = false;
					setup.flipped = true;
					setup.swapchain = true;
					setup.format = state.depthFormat;
					setup.depth = true;

					*outDepthRT = base::RefNew<RenderTargetView>(m_depthTarget->handle(), owningObject, owner()->objectRegistry(), setup);
				}
				else
				{
					*outDepthRT = nullptr;
				}
			}

			return true;
		}

		void Output::disconnect_ClientApi()
		{
			m_swapchain->disconnect_ClientApi();
		}

		void Output::disconnectFromClient()
		{
			m_swapchain->disconnect_ClientApi();
		}

		bool Output::acquire()
		{
			return m_swapchain->acquire();
		}

		void Output::present(bool swap /*= true*/)
		{
			m_swapchain->present(swap);
		}

        //--

        OutputObjectProxy::OutputObjectProxy(ObjectID id, IDeviceObjectHandler* impl, bool flipped, INativeWindowInterface* window, GraphicsPassLayoutObject* layout)
            : rendering::IOutputObject(id, impl, flipped, window, layout)
        {}

        bool OutputObjectProxy::prepare(RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport)
        {
			if (auto* obj = resolveInternalApiObject<Output>())
				return obj->prepare_ClientApi(this, outColorRT, outDepthRT, outViewport);

			return false;
        }

		void OutputObjectProxy::disconnect()
		{
			if (auto* obj = resolveInternalApiObject<Output>())
				return obj->disconnect_ClientApi();
		}

        //--

    } // api
} // rendering
