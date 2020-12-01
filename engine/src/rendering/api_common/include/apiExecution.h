/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingCommands.h"

namespace rendering
{
    namespace api
    {
		//---

		/// helper class to extract temp data from command buffers
		class RENDERING_API_COMMON_API RuntimeDataAllocations : public base::NoCopy
		{
		public:
			RuntimeDataAllocations();

			//--

			uint32_t m_requiredConstantsBuffer = 0;
			uint32_t m_requiredStagingBuffer = 0;

			uint32_t m_constantsDataOffsetInStaging = 0;

			struct Write
			{
				uint32_t offset = 0;
				uint32_t size = 0;
				const void* data = nullptr;
			};

			struct Copy
			{
				uint32_t sourceOffset = 0;
				uint32_t targetOffset = 0;
				uint32_t size = 0;
			};

			struct Mapping
			{
				ObjectID id;
				TransientBufferType type;
				uint32_t offset = 0;
				uint32_t size = 0;
			};

			base::InplaceArray<Write, 1024> m_writes;
			base::InplaceArray<Mapping, 1024> m_mapping;

			base::InplaceArray<Copy, 1024> m_constantBufferCopies;

			uint32_t allocStagingData(uint32_t size);

			void reportConstantsBlockSize(uint32_t size);
			void reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer);
			void reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset);
		};

        //---

		enum class DynamicRenderStatesDirtyBit : uint16_t
		{
			LineWidth,
			DepthBiasValues,
			DepthClampValues,
			StencilFrontCompareMask,
			StencilFrontWriteMask,
			StencilFrontRef,
			StencilBackCompareMask,
			StencilBackWriteMask,
			StencilBackRef,
			BlendColor,
			ViewportRects,
			ScissorRects,
		};

		typedef base::BitFlags<DynamicRenderStatesDirtyBit> DynamicRenderStatesDirtyFlags;

		//---

		struct RENDERING_API_COMMON_API GeometryBufferBinding
		{
			ObjectID id; // buffer ID
			uint32_t offset = 0;

			///--

			GeometryBufferBinding();

			void print(base::IFormatStream& f) const;
		};

		struct RENDERING_API_COMMON_API GeometryStates
		{
			GeometryBufferBinding indexBinding;
			base::Array<GeometryBufferBinding> vertexBindings;

			ImageFormat indexFormat = ImageFormat::UNKNOWN;

			uint8_t maxBoundVertexStreams = 0;
			uint32_t finalIndexStreamOffset = 0;

			//--

			GeometryStates();

			void print(base::IFormatStream& f) const;
		};

		struct RENDERING_API_COMMON_API DescriptorBinding
		{
			base::StringID name;

			const DescriptorEntry* dataPtr = nullptr;
			const DescriptorInfo* layoutPtr = nullptr;

			INLINE operator bool() const { return dataPtr != nullptr; }

			DescriptorBinding();

			void print(base::IFormatStream& f) const;
		};

		struct RENDERING_API_COMMON_API DescriptorState
		{
			base::Array<DescriptorBinding> descriptors;

			INLINE DescriptorState() {};
			INLINE DescriptorState(const DescriptorState& other) : descriptors(other.descriptors) {};
			INLINE DescriptorState(DescriptorState&& other) : descriptors(std::move(other.descriptors)) {};
			INLINE DescriptorState& operator=(const DescriptorState& other) { descriptors = other.descriptors; return *this; }
			INLINE DescriptorState& operator=(DescriptorState&& other) { descriptors = std::move(other.descriptors); return *this; }

			void print(base::IFormatStream& f) const;
		};
		
		struct RENDERING_API_COMMON_API PassAttachmentBinding
		{
			ObjectID viewId; // RTV!

			uint32_t width = 0;
			uint32_t height = 0;

			uint8_t samples = 0;
			ImageFormat format = ImageFormat::UNKNOWN;
			LoadOp loadOp = LoadOp::Keep;
			StoreOp storeOp = StoreOp::Store;

			union
			{
				float valueFloat[4];
				uint32_t valueUint[4];
			} clearValue;

			//--

			PassAttachmentBinding();

			INLINE operator bool() const { return !viewId.empty(); }

			void print(base::IFormatStream& f) const;
		};

		struct RENDERING_API_COMMON_API PassState
		{
			const command::OpBeginPass* passOp = nullptr;

			bool m_valid = false;

			uint8_t samples = 0;
			uint32_t width = 0;
			uint32_t height = 0;

			uint8_t viewportCount = 0;
			uint8_t colorCount = 0;
			
			base::Rect area;

			bool swapchain = false; // we are rendering to a swapchain

			PassAttachmentBinding color[FrameBuffer::MAX_COLOR_TARGETS];
			PassAttachmentBinding depth;

			PassState();

			void print(base::IFormatStream& f) const;
		};

		//---

		class RENDERING_API_COMMON_API IBaseFrameExecutor : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			IBaseFrameExecutor(IBaseThread* thread, Frame* frame, PerformanceStats* stats);
			virtual ~IBaseFrameExecutor();

			INLINE IBaseThread* thread() const { return m_thread; }
			INLINE Frame* frame() const { return m_frame; }
			INLINE PerformanceStats* stats() const { return m_stats; }
			INLINE ObjectRegistry* objects() const { return m_objectRegistry; }
			INLINE IBaseObjectCache* cache() const { return m_objectCache; }

#define RENDER_COMMAND_OPCODE(x) virtual void run##x(const command::Op##x& op) = 0;
#include "rendering/device/include/renderingCommandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE

			void execute(command::CommandBuffer* commandBuffer);

		protected:
			void executeSingle(command::CommandBuffer* commandBuffer);

		private:
			IBaseThread* m_thread = nullptr;
			Frame* m_frame = nullptr;

			ObjectRegistry* m_objectRegistry = nullptr;
			IBaseObjectCache* m_objectCache = nullptr;

			PerformanceStats* m_stats = nullptr;

		};

		//---

		/// state tracker for the executed command buffer
		class RENDERING_API_COMMON_API IFrameExecutor : public IBaseFrameExecutor
		{
		public:
			IFrameExecutor(IBaseThread* thread, Frame* frame, PerformanceStats* stats);
			virtual ~IFrameExecutor();

			void prepare(const RuntimeDataAllocations& data);

		protected:
			PassState m_pass;
			GeometryStates m_geometry;
			DescriptorState m_descriptors;

			base::InplaceArray<DescriptorState, 8> m_descriptorStateStack; // pushed descriptors (for child command buffer processing so nothing leaks)

			IBaseTransientBuffer* m_stagingBuffer = nullptr;
			IBaseTransientBuffer* m_constantBuffer = nullptr;

			ObjectID m_activeSwapchain;
			ObjectID m_activePassLayout;
			bool m_activeSwapchainAcquired = false;

			//--

			bool m_dirtyVertexBuffers = false;
			bool m_dirtyIndexBuffer = false;
			bool m_dirtyDescriptors = false;

			uint64_t m_currentPassKey = 0;
			uint64_t m_currentVertexBindingKey = 0;
			uint64_t m_currentDescriptorBindingKey = 0;
			uint64_t m_currentRenderStateKey = 0;

			//--

			void pushDescriptorState(bool inheritCurrentParameters);
			void popDescriptorState();

			uint32_t printBoundDescriptors() const;

			//--

			static void ExtractPassAttachment(PassState& state, int index, PassAttachmentBinding& att, const FrameBufferColorAttachmentInfo& src);
			static void ExtractPassAttachment(PassState& state, PassAttachmentBinding& att, const FrameBufferDepthAttachmentInfo& src);

			virtual void runNop(const command::OpNop& op) override final;
			virtual void runHello(const command::OpHello& op) override final;
			virtual void runNewBuffer(const command::OpNewBuffer& op) override final;
			virtual void runTriggerCapture(const command::OpTriggerCapture& op) override final;
			virtual void runChildBuffer(const command::OpChildBuffer& op) override final;
			virtual void runAcquireOutput(const command::OpAcquireOutput& op) override final;
			virtual void runSwapOutput(const command::OpSwapOutput& op) override final;

			virtual void runBindVertexBuffer(const command::OpBindVertexBuffer& op) override final;
			virtual void runBindIndexBuffer(const command::OpBindIndexBuffer& op) override final;
			virtual void runBindDescriptor(const command::OpBindDescriptor& op) override final;

			virtual void runUploadConstants(const command::OpUploadConstants& op) override final;
			virtual void runUploadDescriptor(const command::OpUploadDescriptor& op) override final;

			virtual void runBeginPass(const command::OpBeginPass& op) override; // NOT FINAL
			virtual void runEndPass(const command::OpEndPass& op) override; // NOT FINAL
		};

		//--

    } // api
} // rendering