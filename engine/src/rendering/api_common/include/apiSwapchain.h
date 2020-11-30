/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
	namespace api
	{

		//---

		// current swapchain configuration
		struct RENDERING_API_COMMON_API SwapchainState
		{
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t samples = 0;
			ImageFormat colorFormat = ImageFormat::UNKNOWN;
			ImageFormat depthFormat = ImageFormat::UNKNOWN;
			
			bool operator==(const SwapchainState& other) const;
			INLINE bool operator!=(const SwapchainState& other) const { return !operator==(other); }

			void print(base::IFormatStream& f) const;
		};

		//---

		// generalized swapchain, can be owned by window, does not have to
		// NOTE: this object MUST be deleted on render thread
		class RENDERING_API_COMMON_API IBaseSwapchain : public base::NoCopy 
		{
		public:
			IBaseSwapchain(OutputClass cls, bool flipped, INativeWindowInterface* window);
			virtual ~IBaseSwapchain(); // can be destroyed only on render thread

			//--

			// original output class for the swapchain
			INLINE OutputClass outputClass() const { return m_outputClass; }

			// is it flipped vertically (OpenGL crap)
			INLINE bool flipped() const { return m_flipped; }

			// if swapchain has a window this is the abstract interface to it
			INLINE INativeWindowInterface* windowInterface() const { return m_window; }

			//--

			// signal that output will no longer be needed - we still have to finish current rendering but no new frames will be posted
			// NOTE: this also means that any callbacks to client code should not be called any more
			virtual void disconnect_ClientApi() = 0;

			// query swapchain state, it's current format and resolution
			// NOTE: can be called from any thread, should return cached or thread safe data
			virtual bool prepare_ClientApi(SwapchainState& outState) = 0;

			//---

			// query current state
			virtual void queryLayout(GraphicsPassLayoutSetup& outLayout) const = 0;

			// acquire new image from swapchain
			virtual bool acquire() = 0;

			// present swapchain (swap or discard)
			virtual void present(bool swap = true) = 0;

			//--

		public:
			INativeWindowInterface* m_window = nullptr;

			OutputClass m_outputClass = OutputClass::Fullscreen;
			bool m_flipped = false;
		};

		//---

		// generalized swapchain that is based on a window
		// NOTE: this object MUST be deleted on render thread
		class RENDERING_API_COMMON_API IBaseWindowedSwapchain : public IBaseSwapchain
		{
		public:
			struct WindowSetup
			{
				INativeWindowInterface* windowInterface = nullptr;
				WindowManager* windowManager = nullptr;
				uint64_t windowHandle = 0;

				uint64_t deviceHandle = 0;
				ImageFormat colorFormat = ImageFormat::UNKNOWN;
				ImageFormat depthFormat = ImageFormat::UNKNOWN;

				uint8_t samples = 1;
				bool flipped = false;
			};

			IBaseWindowedSwapchain(OutputClass cls, const WindowSetup& setup);
			virtual ~IBaseWindowedSwapchain();

			virtual void disconnect_ClientApi() override;
			virtual bool prepare_ClientApi(SwapchainState& outState) override;
			virtual void queryLayout(GraphicsPassLayoutSetup& outLayout) const override;
	
		protected:
			WindowManager* m_windowManager = nullptr;
			uint64_t m_windowHandle = 0;
			uint64_t m_deviceHandle = 0;
			uint8_t m_samples = 1;

			ImageFormat m_colorFormat = ImageFormat::UNKNOWN;
			ImageFormat m_depthFormat = ImageFormat::UNKNOWN;

			//--

			std::atomic<int> m_disconnected = false;
		};

		//---

	} // api
} // rendering