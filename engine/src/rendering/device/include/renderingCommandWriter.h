/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#pragma once

#include "renderingImageView.h"
#include "renderingConstantsView.h"
#include "renderingParametersView.h"
#include "renderingParametersLayoutID.h"

namespace rendering
{
    struct FrameBufferViewportState;

    namespace command
    {
        struct OpBase;
        struct OpAllocTransientBuffer;
        struct OpBeginPass;

        class CommandBuffer;

        struct AcquiredOutput
        {
            uint32_t width = 0;
            uint32_t height = 0;
            ImageView color;
            ImageView depth;

            INLINE AcquiredOutput() {}
            INLINE operator bool() const { return width != 0 && height != 0; }
        };

        /// helper class for writing the micro operations stream
        /// NOTE: thread safe to write using multiple threads once inside a pass
        class RENDERING_DEVICE_API CommandWriter : public base::NoCopy
        {
        public:
            CommandWriter(CommandBuffer* buffer, base::StringView scopeName = base::StringView()); // NOTE: buffer is reset, that's the only legal way we can write to it
            CommandWriter(base::StringView scopeName = base::StringView()); // NOTE: buffer is reset, that's the only legal way we can write to it
            ~CommandWriter();

            //--

            // release buffer from the writer, finished it as well
            CommandBuffer* release(bool finishRecording = true);

            //--

            // acquire output (back buffer) render targets from render output
            // NOTE: the acquire may fail (if output is lost, if so, rendering should not continue)
            AcquiredOutput opAcquireOutput(IOutputObject* output);

            // swap the previously acquired back buffer
            void opSwapOutput(IOutputObject* output, bool doSwap=true);
            
            //--

            // bind a frame buffer for pixel rendering, only one frame buffer can be bound at any given time
            // NOTE: draw commands only work when there is bound pass
            // NOTE: if we intend to use multiple viewports while rendering we must set it here, optionally we can pass settings for those viewports
            // If settings for viewports are not passed we assume no scissor test and viewport covering full render target area
            void opBeingPass(const FrameBuffer& frameBuffer, uint8_t numViewports = 1, const FrameBufferViewportState* intialViewportSettings = nullptr);

            // finish rendering to a frame buffer, optionally resolve MSSA targets into non-MSAA ones
            void opEndPass();

            /// fill current color render target to single color
            void opClearPassColor(uint32_t index, const base::Vector4& color);

            /// fill current depth/stencil render target to given depth value
            void opClearPassDepthStencil(float depth, uint8_t stencil, bool doClearDepth = true, bool doClearStencil = true);

            /// resolve a MSAA source into a non-msaa destination, average selected samples, for depth targets we can only select one sample
            void opResolve(const ImageView& msaaSource, const ImageView& nonMsaaDest, uint32_t sampleMask = INDEX_NONE, uint8_t depthSampleIndex = 0);

            //---

            /// begin a debug block
            void opBeginBlock(base::StringView name, base::StringView file="", uint32_t line=0);

            /// end debug block
            void opEndBlock();

            //--

            /// create and attach at this point a new command buffer, commands from that command buffer will be inserted here
            /// NOTE: we don't have to fill the command buffer right away and we may do it on a fiber but it must finish recording before we submit work to rendering device
            /// NOTE: the created command buffer inherits currently set descriptors
            CommandBuffer* opCreateChildCommandBuffer(bool inheritParameters=true);

            /// attach external command buffer at this point in the frame sequence
            /// NOTE: we don't have to fill the command buffer right away and we may do it on a fiber but it must finish recording before we submit work to rendering device
            /// NOTE: the created command buffer WILL NOT inherit anything, also, this command buffer cannot be attached while in pass
            void opAttachChildCommandBuffer(CommandBuffer* buffer);

            //--
           
            /// clear buffer with custom value or with zero if no value was provided
            void opClearBuffer(BufferView view, const void* clearValue=nullptr, uint32_t clearValueSize=0);

            /// clear image with custom value or with zero if no value was provided
            void opClearImage(ImageView view, const void* clearValue = nullptr, uint32_t clearValueSize = 0);

            //---

            /// bind vertex buffer at specified slot
            void opBindVertexBuffer(base::StringID name, BufferView buffer, uint32_t offset = 0);

            /// unbind vertex buffer from specified slot
            void opUnbindVertexBuffer(base::StringID name);

            //---

            /// bind index buffer
            void opBindIndexBuffer(BufferView buffer, ImageFormat indexFormat = ImageFormat::R16_UINT, uint32_t offset = 0);

            /// unbind vertex buffer from specified slot
            void opUnbindIndexBuffer();

            //---

            /// change rendering viewport
            /// NOTE: viewport is always reset at the beginning of each pass
            void opSetViewportRect(uint8_t viewportIndex, const base::Rect& viewportRect);
            void opSetViewportRect(uint8_t viewportIndex, int x, int y, int w, int h);

            /// change viewport depth range
            /// NOTE: depth range is always reset at the beginning of each pass
            void opSetViewportDepthRange(uint8_t viewportIndex, float minZ, float maxZ);

            /// set blend state for a given render target
            void opSetBlendState(uint8_t renderTargetIndex, const BlendState& state);
            void opSetBlendState(uint8_t renderTargetIndex); // disable
            void opSetBlendState(uint8_t renderTargetIndex, BlendFactor src, BlendFactor dest);
            void opSetBlendState(uint8_t renderTargetIndex, BlendOp op, BlendFactor src, BlendFactor dest);
            void opSetBlendState(uint8_t renderTargetIndex, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha);

            /// set cull state
            void opSetCullState(const CullState& state);
            void opSetCullState(CullMode mode = CullMode::Back, FrontFace face = FrontFace::CW);

            /// set polygon mode
            void opSetFillState(const FillState& state);
            void opSetFillState(PolygonMode mode = PolygonMode::Fill, float lineWidth=1.0f);

            /// enable/disable scissoring
            void opSetScissorState(bool enabled);

            //// change scissor area
            /// NOTE: scissor is always reset at the beginning of each pass
            void opSetScissorRect(uint8_t viewportIndex, const base::Rect& scissorRect);
            void opSetScissorRect(uint8_t viewportIndex, int x, int y, int w, int h);
            void opSetScissorBounds(uint8_t viewportIndex, int x0, int y0, int x1, int y1);

            /// set the stencil state
            void opSetStencilState(); // disable
            void opSetStencilState(const StencilState& state);
            void opSetStencilState(const StencilSideState& commonFaceState);
            void opSetStencilState(const StencilSideState& frontState, const StencilSideState& backState);
            void opSetStencilState(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp, uint8_t reference = 0, uint8_t compareMask = 0xFF, uint8_t writeMask = 0xFF);

            /// set the stencil reference value
            void opSetStencilReferenceValue(uint8_t value);
            void opSetStencilReferenceValue(uint8_t frontValue, uint8_t backValue);

            /// set stencil compare mask value
            void opSetStencilCompareMask(uint8_t mask);
            void opSetStencilCompareMask(uint8_t frontMask, uint8_t backMask);

            /// set stencil write mask value
            void opSetStencilWriteMask(uint8_t mask);
            void opSetStencilWriteMask(uint8_t frontMask, uint8_t backMask);

            /// set depth state
            void opSetDepthState(const DepthState& state);
            void opSetDepthState(bool enable=true, bool write=true, CompareOp func = CompareOp::LessEqual); // disable

            /// set dynamic depth clip state and ranges
            void opSetDepthClip(bool enabled, float minBounds, float maxBounds);
            void opSetDepthClip(const DepthClipState& state);

            /// set dynamic depth bias clamp
            void opSetDepthBias(const DepthBiasState& state);
            void opSetDepthBias(float constant, float slopeFactor = 0.0f, float clampValue = -1.0f);

            /// set primitive assembly
            void opSetPrimitiveState(const PrimitiveAssemblyState& state);
            void opSetPrimitiveType(PrimitiveTopology topology, bool enableVertexRestart = false);

            /// set multisample state
            void opSetMultisampleState(const MultisampleState& state);

            /// set color mask
            void opSetColorMask(uint8_t rtIndex, uint8_t mask);

            //---

            /// draw non-indexed geometry
            void opDraw(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t vertexCount);

            /// draw non-indexed geometry with instancing
            void opDrawInstanced(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t vertexCount, uint16_t firstInstance, uint16_t numInstances);

            /// draw indexed geometry
            void opDrawIndexed(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount);

            /// draw indexed geometry
            void opDrawIndexedInstanced(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint16_t firstInstance, uint16_t numInstances);

            /// dispatch compute shader
            void opDispatch(const ShaderLibrary* shader, uint32_t countX = 1, uint32_t countY = 1, uint32_t countZ = 1);

            //---
            /// transition image to target layout
            /// NOTE: only the part of the image in the view is transformed
            void opImageLayoutBarrier(const ImageView& view, ImageLayout layout);

            /// create a graphics pipeline barrier telling one stage to wait for results from other stage
            void opGraphicsBarrier(Stage fromStage = Stage::All, Stage toStage = Stage::All);

            //---

            /// update part of dynamic image
            /// the source data can be copied into the command buffer or kept as a separate buffer
            void opUpdateDynamicImage(const ImageView& dynamicImage, const base::image::ImageView& updateData, uint32_t offsetX=0, uint32_t offsetY=0, uint32_t offsetZ=0);

            /// update part of dynamic buffer
            /// the source data can be copied into the command buffer or kept as a separate buffer
            void opUpdateDynamicBuffer(const BufferView& dynamicBuffer, uint32_t dataOffset, uint32_t dataSize, const void* dataPtr);

            /// update part of dynamic buffer
            /// the source data can be copied into the command buffer or kept as a separate buffer
            void* opUpdateDynamicBufferPtr(const BufferView& dynamicBuffer, uint32_t dataOffset, uint32_t dataSize);

            /// update part of dynamic buffer
            /// the source data can be copied into the command buffer or kept as a separate buffer
            template< typename T >
            INLINE T* opUpdateDynamicBufferPtrN(const BufferView& dynamicBuffer, uint32_t first, uint32_t count)
            {
                return (T*)opUpdateDynamicBufferPtr(dynamicBuffer, first * sizeof(T), count * sizeof(T));
            }

            //---

            /// copy data between buffers
            void opCopyBuffer(const BufferView& src, const BufferView& dest, uint32_t srcOffset, uint32_t destOffset, uint32_t size);

            //---

            /// uploads raw constants to command buffer, returns a view on the constant data
            ConstantsView opUploadConstants(const void* data, uint32_t size);

            /// uploads raw constants to command buffer, returns a view on the constant data
            ConstantsView opAllocConstants(uint32_t size, void*& outDataPtr);

            /// uploads raw constants to command buffer, returns a view on the constant data
            template< typename T >
            INLINE ConstantsView opUploadConstants(const T& data)
            {
                static_assert(!std::is_pointer<T>::value, "Pass a reference here, not a pointer");
                return opUploadConstants(&data, sizeof(data));
            }

            /// upload parameters from a buffer of known type
            ParametersView opUploadParameters(const void* data, uint32_t dataSize, ParametersLayoutID layoutID);

            /// upload parameters from a buffer of known type
            template< typename T >
            INLINE ParametersView opUploadParameters(const T& data)
            {
                static_assert(!std::is_pointer<T>::value, "Pass a reference here, not a pointer");
                static auto layoutId  = ParametersLayoutID::FromData(&data, sizeof(T));
                return opUploadParameters(&data, sizeof(T), layoutId);
            }

            //---

            /// bind parameters
            void opBindParameters(base::StringID bindingID, const ParametersView& parameters);

            /// bind parameters from a struct that has the binding ID
            template <typename T>
            INLINE void opBindParametersInline(base::StringID name, const T &parameters)
            {
                // the inline parameters are assumed to be unique, upload them and bind
                static auto layoutId  = ParametersLayoutID::FromData(&parameters, sizeof(T));
                opBindParameters(name, opUploadParameters(&parameters, sizeof(parameters), layoutId));
            }

            //---

            /// download content of buffer resource from the GPU
            /// NOTE: the download does not complete immediately, use the isReady() or waitUntilReady()
            void opDownloadBuffer(const BufferView& buffer, const DownloadBufferPtr& ptr);

            /// download content of image resource from the GPU
            /// NOTE: the download does not complete immediately, use the isReady() or waitUntilReady()
            /// NOTE: only first mipmap and slice of the image is downloaded, use multiple downloads to get the whole thing for now
            void opDownloadImage(const ImageView& image, const DownloadImagePtr& ptr);

            //---

            /// trigger capture HACK for render doc
            void opTriggerCapture();

        private:
            void ensureMemory(uint32_t size);

            INLINE void* allocMemory(uint32_t size)
            {
                ensureMemory(size);

                auto* ret = m_writePtr;
                m_writePtr += size;
                return ret;
            }

            template< typename T >
            T* allocCommand(uint32_t extraSize = 0)
            {
                ensureMemory(sizeof(T) + extraSize);
                return T::Alloc<T>(m_writePtr, extraSize, m_lastCommand);
            }

            //---

            OpBase* m_lastCommand = nullptr;

            uint8_t* m_writePtr = nullptr;
            uint8_t* m_writeEndPtr = nullptr;
            CommandBuffer* m_writeBuffer = nullptr;

            OpBeginPass* m_currentPass = nullptr;
            uint32_t m_numOpenedBlocks = 0;
            uint32_t m_currentPassRts = 0;
            uint32_t m_currentPassViewports = 0;

            uint32_t m_currentIndexBufferElementCount = 0;
            base::HashMap<base::StringID, uint32_t> m_currentVertexBufferRemainingSize;
            base::HashMap<base::StringID, ParametersLayoutID> m_currentParameterBindings;

            bool m_isChildBufferWithParentPass = false;

            void attachBuffer(CommandBuffer* buffer);
            void detachBuffer(bool finishRecording);

            bool validateDrawVertexLayout(const ShaderLibrary* func, uint32_t requiredVertexCount) const;
            bool validateParameterBindings(const ShaderLibrary* func) const;
        };

        //--

        // scoped command block
        struct CommandWriterBlock : public base::NoCopy
        {
        public:
            INLINE CommandWriterBlock(CommandWriter& cmd, base::StringView name)
            {
                if (name)
                {
                    m_writer = &cmd;
                    cmd.opBeginBlock(name);
                }
            }

            INLINE ~CommandWriterBlock()
            {
                if (m_writer)
                    m_writer->opEndBlock();
            }

        private:
            CommandWriter* m_writer = nullptr;
        };

        //--

    } // command
} // rendering