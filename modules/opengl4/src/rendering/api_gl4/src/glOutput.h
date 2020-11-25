/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\output #]
***/

#pragma once

#include "rendering/device/include/renderingOutput.h"
#include "rendering/api_common/include/renderingWindow.h"

#include "glObject.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {

        //---

        // output render target view
		class OutputRenderTarget : public Object
		{
		public:
			OutputRenderTarget(Device* drv, Output* output, ImageFormat format, uint8_t samples, bool depth);

			// target swapchain
			INLINE Output* output() const { return m_output; }

			// output format
			INLINE ImageFormat format() const { return m_format; }

			// number of samples (MSAA outputs)
			INLINE uint8_t samples() const { return m_samples; }

			// is this a depth buffer 
			INLINE bool depth() const { return m_depth; }

		private:
			Output* m_output = nullptr;

			ImageFormat m_format; // determines 
			uint8_t m_samples = 1;
			bool m_depth = false;
		};

        // an output object - OpenGL "swapchain" kind of object
        class Output : public Object
        {
        public:
            Output(Device* drv, DeviceThread* th, WindowManager* windows, OutputClass cls, bool flipped);
            virtual ~Output();

            static const auto STATIC_TYPE = ObjectType::Output;

            INLINE OutputClass outputClass() const { return m_outputClass; }
            INLINE bool flipped() const { return m_flipped; }

            INLINE uint64_t windowHandle() const { return m_windowHandle; }
            INLINE uint64_t deviceHandle() const { return m_deviceHandle; }

            INLINE INativeWindowInterface* windowInterface() const { return m_windowInterface; }

            //---

            void bind(uint64_t windowHandle, uint64_t deviceHandle, ImageFormat colorFormat, ImageFormat depthFormat, uint8_t numSamples, INativeWindowInterface* window);

            bool prepare(IDeviceObject* owner, RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport);

            //--

        protected:
            OutputClass m_outputClass;
            bool m_flipped = false;

            WindowManager* m_windows = nullptr;
            DeviceThread* m_thread = nullptr;

			OutputRenderTarget* m_colorTarget = nullptr;
			OutputRenderTarget* m_depthTarget = nullptr;

			struct {
				uint32_t lastWidth = 0;
				uint32_t lastHeight = 0;
				RenderTargetViewPtr color;
				RenderTargetViewPtr depth;
			} m_renderTargets;

            uint64_t m_windowHandle = 0; // HWND
            uint64_t m_deviceHandle = 0; // HDC

            INativeWindowInterface* m_windowInterface = nullptr;
        };

        //---

        // engine side proxy for device output
        class OutputObjectProxy : public rendering::IOutputObject
        {
        public:
            OutputObjectProxy(ObjectID id, ObjectRegistryProxy* impl, bool flipped, INativeWindowInterface* window);

            virtual bool prepare(RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport) override final;

        private:
            base::RefWeakPtr<ObjectRegistryProxy> m_proxy;
        };

        //---

    } // gl4
} // rendering