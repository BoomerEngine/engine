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
			m_colorRTV.release();
			m_depthRTV.release();

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

			// recreate ?
			if (state != m_lastSwapchainState)
			{
				TRACE_INFO("Recreating render targets for swapchain {}", state);

				m_colorRTV.reset();
				m_depthRTV.reset();

				m_lastSwapchainState = state;

				if (state.width && state.height)
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

					if (state.colorFormat != ImageFormat::UNKNOWN)
					{
						setup.format = state.colorFormat;
						setup.depth = false;
						m_colorRTV = base::RefNew<RenderTargetView>(m_colorTarget->handle(), owningObject, owner()->objectRegistry(), setup);
					}

					if (state.depthFormat != ImageFormat::UNKNOWN)
					{
						setup.format = state.depthFormat;
						setup.depth = true;
						m_depthRTV = base::RefNew<RenderTargetView>(m_depthTarget->handle(), owningObject, owner()->objectRegistry(), setup);
					}
				}
			}

			if (!m_lastSwapchainState.width || !m_lastSwapchainState.height)
				return false;

			outViewport.x = m_lastSwapchainState.width;
			outViewport.y = m_lastSwapchainState.height;

			if (outColorRT)
				*outColorRT = m_colorRTV;

			if (outDepthRT)
				*outDepthRT = m_depthRTV;

			return true;
		}

		void Output::disconnectFromClient()
		{
			TRACE_INFO("Disconnected swapchain {}, some rendering may still happen", m_lastSwapchainState);
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

        //--

    } // api
} // rendering
