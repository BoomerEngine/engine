/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#pragma once

#include "rendering/device/include/renderingCommands.h"

#include "glBuffer.h"
#include "glImage.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            /// "dirty" state type, used as a bit mask to restore state between command buffers
            enum class DirtyStateBit : uint64_t
            {
                VertexBuffers,
                IndexBuffer,
                DepthBias,
                FillState,
                PrimitiveState,
                CullState,
                BlendColor,
                BlendState,
                DepthState,
                DepthClipRange,
                StencilState,
                ColorMask,
                Parameters,
            };

            typedef base::BitFlags<DirtyStateBit> DirtyStateFlags;

            //----

            /// helper class to extract temp data from command buffers
            class RuntimeDataAllocations : public base::NoCopy
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
                    TempBufferType targetType;
                    uint32_t sourceOffset = 0;
                    uint32_t targetOffset = 0;
                    uint32_t size = 0;
                };

                struct Mapping
                {
                    ObjectID id;
                    TempBufferType type;
                    uint32_t offset = 0;
                    uint32_t size = 0;
                };

                base::InplaceArray<Write, 1024> m_writes;
                base::InplaceArray<Copy, 1024> m_copies;
                base::InplaceArray<Mapping, 1024> m_mapping;

                uint32_t allocStagingData(uint32_t size);

                void reportConstantsBlockSize(uint32_t size);
                void reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer);
                void reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset);
            };

            //----

            class RuntimeBindingState : public base::NoCopy
            {
            public:
                RuntimeBindingState();

                void bindUniformBuffer(uint32_t slot, const ResolvedBufferView& buffer);
                void bindStorageBuffer(uint32_t slot, const ResolvedBufferView& buffer);
                void bindImage(uint32_t slot, const ResolvedFormatedView& buffer, GLuint mode);
                void bindTexture(uint32_t slot, const ResolvedImageView& view);
                void bindSampler(uint32_t slot, GLuint samplerView);

                void reset();

            private:
                static const uint32_t MAX_SLOTS = 32;
                ResolvedBufferView m_currentUniformBuffers[MAX_SLOTS];
                ResolvedBufferView m_currentStorageBuffers[MAX_SLOTS];

                ResolvedFormatedView m_currentImages[MAX_SLOTS];
                GLuint m_currentImagesReadWriteMode[MAX_SLOTS];

                GLuint m_currentTextures[MAX_SLOTS];
                GLuint m_currentSamplers[MAX_SLOTS];
            };

            //---

            /// state tracker for the executed command buffer
            class FrameExecutor : public base::NoCopy
            {
            public:
                FrameExecutor(Device* drv, DeviceThread* thread, Frame* frame, const RuntimeDataAllocations& data, PerformanceStats* stats);
                ~FrameExecutor();

                INLINE PerformanceStats& stats() { return *m_stats; }

                void pushParamState(bool inheritCurrentParameters);
                void popParamState();
                void printParamState();

    #define RENDER_COMMAND_OPCODE(x) void run##x(const command::Op##x& op);
    #include "rendering/device/include/renderingCommandOpcodes.inl"
    #undef RENDER_COMMAND_OPCODE

            private:
                Device& m_device;
                DeviceThread& m_thread;
                Frame& m_frame;

                ObjectCache& m_objectCache;
                ObjectRegistry& m_objectRegistry;

                uint32_t m_nextDebugMessageID = 1;

                //--

                struct GeometryState
                {
                    base::Array<BufferView> vertexBindings;
                    uint8_t maxBoundVertexStreams = 0;

                    uint32_t indexBufferOffset = 0;
                    BufferView indexStreamBinding;
                    ImageFormat indexFormat = ImageFormat::UNKNOWN;

                    bool indexBindingsChanged = false;
                    bool vertexBindingsChanged = false;
                };

                struct ParamsState
                {
                    base::Array<ParametersView> paramBindings;
                    bool parameterBindingsChanged = false;
                };

                struct PassState
                {
                    const command::OpBeginPass* passOp = nullptr;
                    GLuint fbo = 0;
                    uint8_t colorCount = 0;
                    uint8_t viewportCount = 0;
                    uint32_t width = 0;
                    uint32_t height = 0;
                    uint32_t samples = 0;
                };

                enum class RenderStateDirtyBit : uint64_t
                {
                    ViewportRects,
                    ViewportDepthRanges,
                    ScissorEnabled,
                    ScissorRects,
                    StencilEnabled,
                    StencilFrontFuncReferenceMask,
                    StencilBackFuncReferenceMask,
                    StencilFrontWriteMask,
                    StencilBackWriteMask,
                    StencilFrontOps,
                    StencilBackOps,
                    DepthBiasEnabled,
                    DepthBiasValues,
                    PolygonFillMode,
                    PolygonLineWidth,
                    PolygonPrimitiveRestart,
                    PolygonTopology,
                    CullFrontFace,
                    CullMode,
                    CullEnabled,
                    BlendingEnabled,
                    BlendingFunc,
                    BlendingEquation,
                    BlendingColor,
                    ColorMask,
                    DepthEnabled,
                    DepthWrite,
                    DepthFunc,
                    DepthBoundsEnabled,
                    DepthBoundsRanges,
                    AlphaCoverageEnabled,
                    AlphaCoverageDitherEnabled,

                    MAX,
                };

                typedef base::BitFlags<RenderStateDirtyBit> RenderStateDirtyFlags;

                struct RenderDirtyStateTrack
                {
                    RenderStateDirtyFlags flags;
                    uint8_t blendEquationDirtyPerRT = 0; // per RT state
                    uint8_t blendFuncDirtyPerRT = 0; // per RT state
                    uint8_t colorMaskDirtyPerRT = 0; // per RT state
                    uint16_t scissorDirtyPerVP = 0; // per viewport state
                    uint16_t viewportDirtyPerVP = 0; // per viewport state
                    uint16_t depthRangeDirtyPerVP = 0; // per viewport state

                    RenderDirtyStateTrack& operator|=(const RenderDirtyStateTrack& other);

                    static RenderDirtyStateTrack ALL_STATES();
                };

                struct RenderStates
                {
                    struct ResolvedStencilFaceState
                    {
                        GLenum func = GL_ALWAYS; 
                        uint8_t ref = 0;
                        uint8_t compareMask = 0xFF;
                        uint8_t writeMask = 0xFF;
                        GLenum failOp = GL_KEEP;
                        GLenum depthFailOp = GL_KEEP;
                        GLenum passOp = GL_KEEP;
                    };

                    struct ResolvedStencilState
                    {
                        ResolvedStencilFaceState front;
                        ResolvedStencilFaceState back;
                        bool enabled = false;
                    };

                    struct ResolvedBlendState
                    {
                        GLenum colorOp = GL_FUNC_ADD;
                        GLenum colorSrc = GL_ONE;
                        GLenum colorDest = GL_ZERO;
                        GLenum alphaOp = GL_FUNC_ADD;
                        GLenum alphaSrc = GL_ONE;
                        GLenum alphaDest = GL_ZERO;

                        INLINE bool enabled() const
                        {
                            return (colorSrc != GL_ONE) || (colorDest != GL_ZERO) 
                                || (alphaSrc != GL_ONE) || (alphaDest != GL_ZERO) 
                                || (colorOp != GL_FUNC_ADD) || (alphaOp != GL_FUNC_ADD);
                        }
                    };

                    struct ResolvedViewportState
                    {
                        GLfloat rect[4] = { 0,0,0,0 };
                        GLint scissor[4] = { 0,0,0,0 };
                        float depthMin = 0.0f;
                        float depthMax = 1.0f;
                    };

                    struct ResolvedRTState
                    {
                        ResolvedBlendState blend;
                        uint8_t colorMask = 0xF;
                    };

                    struct ResolvedDepthState
                    {
                        bool enabled = false;
                        bool writeEnabled = true;
                        GLenum func = GL_ALWAYS;
                    };

                    struct ResolvedDepthClipState
                    {
                        bool enabled = false;
                        float min = 0.0f;
                        float max = 1.0f;
                    };

                    struct ResolvedDepthBiasState
                    {
                        bool biasEnabled = false;
                        float constant = 0.0f;
                        float slope = 0.0f;
                        float clamp = 0.0f;
                    };

                    struct ResolvePolygonState
                    {
                        GLenum fill = GL_FILL;
                        GLenum topology = GL_TRIANGLES;
                        float lineWidth = 1.0f;
                        bool restartEnabled = false;
                    };

                    struct ResolvedCullState
                    {
                        GLenum front = GL_CW;
                        GLenum cull = GL_BACK;
                        bool enabled = false;
                    };

                    struct ResolvedMultisampleState
                    {
                        bool alphaToCoverageEnabled = false;
                        bool alphaToOneEnabled = false;
                        bool alphaCoverageDitherEnabled = false;
                    };

                    static const uint8_t MAX_VIEWPORTS = 16;
                    static const uint8_t MAX_RTS = 8;

                    static const uint32_t VIEWPORT_MASK = (1U << MAX_VIEWPORTS) - 1;
                    static const uint32_t RT_MASK = (1U << MAX_RTS) - 1;

                    ResolvePolygonState polygon;
                    ResolvedDepthState depth;
                    ResolvedDepthBiasState depthBias;
                    ResolvedDepthClipState depthClip;
                    ResolvedStencilState stencil;
                    ResolvedCullState cull;
                    ResolvedViewportState viewport[MAX_VIEWPORTS];
                    ResolvedMultisampleState multisample;
                    ResolvedRTState rt[MAX_VIEWPORTS];

                    bool scissorEnabled = false;
                    bool blendingEnabled = false;
                    float blendColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

                    void apply(RenderDirtyStateTrack statesToApply) const;
                };

                static const RenderStates GDefaultState;

                struct ProgramState
                {
                    ObjectID activeDrawShaderLibrary;
                    PipelineIndex activeDrawShaderBundleIndex = INVALID_PIPELINE_INDEX;
                    GLuint glActiveDrawLinkedProgramObject = 0;

                    ObjectID activeComputeShaderLibrary;
                    PipelineIndex activeDispatchShaderBundleIndex = INVALID_PIPELINE_INDEX;
                    GLuint glActiveDispatchLinkedProgramObject = 0;

                    GLuint glActiveProgram = 0;
                };

                //--

                GeometryState m_geometry;
                PassState m_pass;
                ParamsState m_params;
                ProgramState m_program;
                RuntimeBindingState m_objectBindings;

                RenderStates m_render;
                RenderDirtyStateTrack m_dirtyRenderStates;
                RenderDirtyStateTrack m_passChangedRenderStates;

                TempBuffer* m_tempConstantBuffer = nullptr;
                TempBuffer* m_tempStagingBuffer = nullptr;

                static bool SetStencilFunc(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op);
                static bool SetStencilWriteMask(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op);
                static bool SetStencilOps(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op);

                //--

                ResolvedBufferView resolveUntypedBufferView(const rendering::BufferView& view);
                ResolvedFormatedView resolveTypedBufferView(const rendering::BufferView& view, ImageFormat format);
                ResolvedImageView resolveImageView(const rendering::ImageView& view);

                GLuint resolveSampler(ObjectID id);

                //--

                void updateBlendingEnable();
                void resetCommandBufferRenderState();

                void prepareFrameData(const RuntimeDataAllocations& data);

                void applyDirtyRenderStates();
                void applyViewportScissorState(uint8_t index, const FrameBufferViewportState& vs, bool& usesStencil);
                bool applyVertexData(const Shaders& shaders, PipelineIndex vertexStateIndex);
                bool applyParameters(const Shaders& shaders, PipelineIndex parameterBindingStateIndex);
                bool applyIndexData();

                bool prepareDraw(const Shaders& shaders, PipelineIndex linkedProgramIndex, bool usesIndices);
                bool prepareDispatch(const Shaders& shaders, PipelineIndex linkedProgramIndex);

                //--

                base::Array<ParamsState> m_paramStateStack;

                //--

                struct ActiveSwapchains
                {
                    ObjectID colorRT;
                    ObjectID depthRT;
                    ObjectID swapchain;
                };

                base::InplaceArray<ActiveSwapchains, 4> m_activeSwapchains;

                //--

                PerformanceStats* m_stats;
            };

        } // exec
    } // gl4
} // rendering

