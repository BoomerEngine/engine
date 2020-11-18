/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glDevice.h"
#include "glUtils.h"
#include "glBuffer.h"
#include "glImage.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {
            //---

            static GLenum TranslateCompareOp(const CompareOp op)
            {
                switch (op)
                {
                case CompareOp::Never: return GL_NEVER;
                case CompareOp::Less: return GL_LESS;
                case CompareOp::LessEqual: return GL_LEQUAL;
                case CompareOp::Greater: return GL_GREATER;
                case CompareOp::Equal: return GL_EQUAL;
                case CompareOp::NotEqual: return GL_NOTEQUAL;
                case CompareOp::GreaterEqual: return GL_GEQUAL;
                case CompareOp::Always: return GL_ALWAYS;
                }

                TRACE_ERROR("Invalid compare op");
                return GL_ALWAYS;
            }

            static GLenum TranslateBlendFactor(const BlendFactor op)
            {
                switch (op)
                {
                case BlendFactor::Zero: return GL_ZERO;
                case BlendFactor::One: return GL_ONE;
                case BlendFactor::SrcColor: return GL_SRC_COLOR;
                case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
                case BlendFactor::DestColor: return GL_DST_COLOR;
                case BlendFactor::OneMinusDestColor: return GL_ONE_MINUS_DST_COLOR;
                case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
                case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DestAlpha: return GL_DST_ALPHA;
                case BlendFactor::OneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
                case BlendFactor::ConstantColor: return GL_CONSTANT_COLOR;
                case BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
                case BlendFactor::ConstantAlpha: return GL_CONSTANT_ALPHA;
                case BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
                case BlendFactor::SrcAlphaSaturate: return GL_SRC_ALPHA_SATURATE;
                case BlendFactor::Src1Color: return GL_SRC1_COLOR;
                case BlendFactor::OneMinusSrc1Color: return GL_ONE_MINUS_SRC1_COLOR;
                case BlendFactor::Src1Alpha: return GL_SRC1_ALPHA;
                case BlendFactor::OneMinusSrc1Alpha: return GL_ONE_MINUS_SRC1_ALPHA;
                }

                TRACE_ERROR("Invalid blend factor");
                return GL_ONE;
            }

            static GLenum TranslateBlendOp(const BlendOp op)
            {
                switch (op)
                {
                case BlendOp::Add: return GL_FUNC_ADD;
                case BlendOp::Subtract: return GL_FUNC_SUBTRACT;
                case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
                case BlendOp::Min: return GL_MIN;
                case BlendOp::Max: return GL_MAX;
                }

                TRACE_ERROR("Invalid bend op");
                return GL_ADD;

            }

            static GLenum TranslateStencilOp(const StencilOp op)
            {
                switch (op)
                {
                case StencilOp::Keep: return GL_KEEP;
                case StencilOp::Zero: return GL_ZERO;
                case StencilOp::Replace: return GL_REPLACE;
                case StencilOp::IncrementAndClamp: return GL_INCR;
                case StencilOp::DecrementAndClamp: return GL_DECR;
                case StencilOp::Invert: GL_INVERT;
                case StencilOp::IncrementAndWrap: return GL_INCR_WRAP;
                case StencilOp::DecrementAndWrap: return GL_DECR_WRAP;
                }

                TRACE_ERROR("Invalid stencil op");
                return GL_KEEP;
            }

            static GLenum TranslateLogicOp(const LogicalOp op)
            {
                switch (op)
                {
                case LogicalOp::Clear: return GL_CLEAR;
                case LogicalOp::And: return GL_AND;
                case LogicalOp::AndReverse: return GL_AND_REVERSE;
                case LogicalOp::Copy: return GL_COPY;
                case LogicalOp::AndInverted: return GL_AND_INVERTED;
                case LogicalOp::NoOp: return GL_NOOP;
                case LogicalOp::Xor: return GL_XOR;
                case LogicalOp::Or: return GL_OR;
                case LogicalOp::Nor: return GL_NOR;
                case LogicalOp::Equivalent: return GL_EQUIV;
                case LogicalOp::Invert: return GL_INVERT;
                case LogicalOp::OrReverse: return GL_OR_REVERSE;
                case LogicalOp::CopyInverted: return GL_COPY_INVERTED;
                case LogicalOp::OrInverted: return GL_OR_INVERTED;
                case LogicalOp::Nand: return GL_NAND;
                case LogicalOp::Set: return GL_SET;
                }

                TRACE_ERROR("Invalid logical op");
                return GL_NOOP;
            }

            static GLenum TranslateDrawTopology(const PrimitiveTopology topology)
            {
                switch (topology)
                {
                case PrimitiveTopology::PointList: return GL_POINTS;
                case PrimitiveTopology::LineList: return GL_LINES;
                case PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
                case PrimitiveTopology::TriangleList: return GL_TRIANGLES;
                case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
                case PrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
                case PrimitiveTopology::LineListWithAdjacency: return GL_LINES_ADJACENCY;
                case PrimitiveTopology::LineStripWithAdjacency: return GL_LINE_STRIP_ADJACENCY;
                case PrimitiveTopology::TriangleListWithAdjacency: return GL_TRIANGLES_ADJACENCY;
                case PrimitiveTopology::TriangleStripWithAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
                case PrimitiveTopology::PatchList: return GL_PATCHES;
                }

                FATAL_ERROR("Invalid topology");
                return GL_TRIANGLES;
            }

            //---
        
            static void glEnableState(GLenum val, bool state)
            {
                if (state)
                    glEnable(val);
                else
                    glDisable(val);
            }

            FrameExecutor::RenderDirtyStateTrack& FrameExecutor::RenderDirtyStateTrack::operator|=(const RenderDirtyStateTrack& other)
            {
                flags |= other.flags;
                blendEquationDirtyPerRT |= other.blendEquationDirtyPerRT;
                blendFuncDirtyPerRT |= other.blendFuncDirtyPerRT;
                colorMaskDirtyPerRT |= other.colorMaskDirtyPerRT;
                scissorDirtyPerVP |= other.scissorDirtyPerVP;
                viewportDirtyPerVP |= other.viewportDirtyPerVP;
                depthRangeDirtyPerVP |= other.depthRangeDirtyPerVP;
                return *this;
            }

            FrameExecutor::RenderDirtyStateTrack FrameExecutor::RenderDirtyStateTrack::ALL_STATES()
            {
                RenderDirtyStateTrack ret;
                ret.blendEquationDirtyPerRT = RenderStates::RT_MASK;
                ret.blendFuncDirtyPerRT = RenderStates::RT_MASK;
                ret.colorMaskDirtyPerRT = RenderStates::RT_MASK;
                ret.depthRangeDirtyPerVP = RenderStates::VIEWPORT_MASK;
                //ret.scissorDirtyPerVP = RenderStates::VIEWPORT_MASK;
                //ret.viewportDirtyPerVP = RenderStates::VIEWPORT_MASK; ?

                static const auto ALL_FLAGS_MASK = (1ULL << (uint64_t)RenderStateDirtyBit::MAX) - 1;
                ret.flags = RenderStateDirtyFlags(ALL_FLAGS_MASK);
                return ret;
            }

            void FrameExecutor::RenderStates::apply(RenderDirtyStateTrack statesToApply) const
            {
                PC_SCOPE_LVL2(ApplyRenderStates);

                auto flags = statesToApply.flags;

                // TODO: sort from most commonly set

                if (flags.test(RenderStateDirtyBit::StencilFrontFuncReferenceMask))
                    GL_PROTECT(glStencilFuncSeparate(GL_FRONT, stencil.front.func, stencil.front.ref, stencil.front.compareMask));

                if (flags.test(RenderStateDirtyBit::StencilBackFuncReferenceMask))
                    GL_PROTECT(glStencilFuncSeparate(GL_BACK, stencil.front.func, stencil.front.ref, stencil.front.compareMask));

                if (flags.test(RenderStateDirtyBit::StencilFrontOps))
                    GL_PROTECT(glStencilOpSeparate(GL_FRONT, stencil.front.failOp, stencil.front.depthFailOp, stencil.front.passOp));

                if (flags.test(RenderStateDirtyBit::StencilBackOps))
                    GL_PROTECT(glStencilOpSeparate(GL_BACK, stencil.back.failOp, stencil.back.depthFailOp, stencil.back.passOp));

                if (flags.test(RenderStateDirtyBit::StencilFrontWriteMask))
                    GL_PROTECT(glStencilMaskSeparate(GL_FRONT, stencil.front.writeMask));

                if (flags.test(RenderStateDirtyBit::StencilBackWriteMask))
                    GL_PROTECT(glStencilMaskSeparate(GL_BACK, stencil.front.writeMask));

                if (flags.test(RenderStateDirtyBit::StencilEnabled))
                    GL_PROTECT(glEnableState(GL_STENCIL_TEST, stencil.enabled));

                if (flags.test(RenderStateDirtyBit::DepthEnabled))
                    GL_PROTECT(glEnableState(GL_DEPTH_TEST, depth.enabled));

                if (flags.test(RenderStateDirtyBit::DepthWrite))
                    GL_PROTECT(glDepthMask(depth.writeEnabled));

                if (flags.test(RenderStateDirtyBit::DepthFunc))
                    GL_PROTECT(glDepthFunc(depth.func));

                if (flags.test(RenderStateDirtyBit::DepthBiasEnabled))
                {
                    GL_PROTECT(glEnableState(GL_POLYGON_OFFSET_FILL, depthBias.biasEnabled));
                    GL_PROTECT(glEnableState(GL_POLYGON_OFFSET_LINE, depthBias.biasEnabled));
                }

                if (flags.test(RenderStateDirtyBit::DepthBiasValues))
                    GL_PROTECT(glPolygonOffsetClamp(depthBias.slope, depthBias.constant, depthBias.clamp));

                if (flags.test(RenderStateDirtyBit::ScissorEnabled))
                    GL_PROTECT(glEnableState(GL_SCISSOR_TEST, scissorEnabled));

                if (flags.test(RenderStateDirtyBit::ScissorRects))
                {
                    auto mask = statesToApply.scissorDirtyPerVP & VIEWPORT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);
                        GL_PROTECT(glScissorIndexedv(index, &viewport[index].scissor[0]));
                        mask ^= mask & -mask;
                    }
                }

                if (flags.test(RenderStateDirtyBit::ViewportRects))
                {
                    auto mask = statesToApply.viewportDirtyPerVP & VIEWPORT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);
                        GL_PROTECT(glViewportIndexedfv(index, &viewport[index].rect[0]));
                        mask ^= mask & -mask;
                    }
                }

                if (flags.test(RenderStateDirtyBit::ViewportDepthRanges))
                {
                    auto mask = statesToApply.depthRangeDirtyPerVP & VIEWPORT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);
                        GL_PROTECT(glDepthRangeIndexed(index, viewport[index].depthMin, viewport[index].depthMax));
                        mask ^= mask & -mask;
                    }
                }

                if (flags.test(RenderStateDirtyBit::PolygonFillMode))
                    GL_PROTECT(glPolygonMode(GL_FRONT_AND_BACK, polygon.fill));

                if (flags.test(RenderStateDirtyBit::PolygonLineWidth))
                    GL_PROTECT(glLineWidth(polygon.lineWidth));

                if (flags.test(RenderStateDirtyBit::PolygonPrimitiveRestart))
                    GL_PROTECT(glEnableState(GL_PRIMITIVE_RESTART_FIXED_INDEX, polygon.restartEnabled));

                if (flags.test(RenderStateDirtyBit::PolygonTopology))
                    GL_PROTECT(glEnableState(GL_PROGRAM_POINT_SIZE, polygon.topology == GL_POINTS));

                if (flags.test(RenderStateDirtyBit::CullFrontFace))
                    GL_PROTECT(glFrontFace(cull.front));

                if (flags.test(RenderStateDirtyBit::CullMode))
                    GL_PROTECT(glCullFace(cull.cull));

                if (flags.test(RenderStateDirtyBit::CullEnabled))
                    GL_PROTECT(glEnableState(GL_CULL_FACE, cull.enabled));

                if (flags.test(RenderStateDirtyBit::BlendingEnabled))
                    GL_PROTECT(glEnableState(GL_BLEND, blendingEnabled));

                if (flags.test(RenderStateDirtyBit::BlendingEquation))
                {
                    auto mask = statesToApply.blendEquationDirtyPerRT & RT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);

                        const auto& bs = this->rt[index].blend;

                        if (bs.alphaOp == bs.colorOp)
                            GL_PROTECT(glBlendEquationi(index, bs.colorOp))
                        else
                            GL_PROTECT(glBlendEquationSeparatei(index, bs.colorOp, bs.alphaOp));
                        mask ^= mask & -mask;
                    }            
                }

                if (flags.test(RenderStateDirtyBit::BlendingFunc))
                {
                    auto mask = statesToApply.blendFuncDirtyPerRT & RT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);

                        const auto& bs = this->rt[index].blend;

                        if (bs.colorSrc == bs.alphaSrc && bs.colorDest == bs.alphaDest)
                            GL_PROTECT(glBlendFunci(index, bs.colorSrc, bs.colorDest))
                        else
                            GL_PROTECT(glBlendFuncSeparatei(index, bs.colorSrc, bs.colorDest, bs.alphaSrc, bs.alphaDest));
                        mask ^= mask & -mask;
                    }
                }

                if (flags.test(RenderStateDirtyBit::BlendingColor))
                {
                    GL_PROTECT(glBlendColor(blendColor[0], blendColor[1], blendColor[2], blendColor[3]));
                }

                if (flags.test(RenderStateDirtyBit::ColorMask))
                {
                    auto mask = statesToApply.colorMaskDirtyPerRT & RT_MASK;
                    while (mask)
                    {
                        uint32_t index = __builtin_ctzll(mask);

                        const auto colorMask = this->rt[index].colorMask;

                        GL_PROTECT(glColorMaski(index, colorMask & 1, colorMask & 2, colorMask & 4, colorMask & 8));
                        mask ^= mask & -mask;
                    }
                }

                if (flags.test(RenderStateDirtyBit::DepthBoundsEnabled))
                    GL_PROTECT(glEnableState(GL_DEPTH_CLAMP, depthClip.enabled));

                if (flags.test(RenderStateDirtyBit::DepthBoundsRanges))
                    GL_PROTECT(glDepthBoundsEXT(depthClip.min, depthClip.max));

                if (flags.test(RenderStateDirtyBit::AlphaCoverageEnabled))
                    GL_PROTECT(glEnableState(GL_SAMPLE_ALPHA_TO_COVERAGE, multisample.alphaToCoverageEnabled));

                if (flags.test(RenderStateDirtyBit::AlphaCoverageDitherEnabled))
                    GL_PROTECT(glAlphaToCoverageDitherControlNV(multisample.alphaCoverageDitherEnabled ? GL_ALPHA_TO_COVERAGE_DITHER_ENABLE_NV : GL_ALPHA_TO_COVERAGE_DITHER_DISABLE_NV));            
            }

            const FrameExecutor::RenderStates FrameExecutor::GDefaultState;
        
            //---

    #define DEBUG_CHECK_PASS_ONLY() \
            DEBUG_CHECK_EX(m_pass.passOp != nullptr, "Not in pass"); \
            if (m_pass.passOp == nullptr) return;

            void FrameExecutor::runSetViewportRect(const command::OpSetViewportRect& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                DEBUG_CHECK_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");
                if (op.viewportIndex < m_pass.viewportCount)
                {
                    auto& targetRect = m_render.viewport[op.viewportIndex].rect;

                    const float x = op.rect.left();
                    //const float y = op.rect.top();
                    const float y = (int)m_pass.height - op.rect.top() - op.rect.height();
                    const float w = op.rect.width();
                    const float h = op.rect.height();

                    if (targetRect[0] != x || targetRect[1] != y || targetRect[2] != w || targetRect[3] != h)
                    {
                        targetRect[0] = x;
                        targetRect[1] = y;
                        targetRect[2] = w;
                        targetRect[3] = h;

                        m_dirtyRenderStates.viewportDirtyPerVP |= (1UL << op.viewportIndex);
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ViewportRects;
                        m_passChangedRenderStates.viewportDirtyPerVP |= (1UL << op.viewportIndex);
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ViewportRects;
                    }
                }
            }

            void FrameExecutor::runSetViewportDepthRange(const command::OpSetViewportDepthRange& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                DEBUG_CHECK_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");
                if (op.viewportIndex < m_pass.viewportCount)
                {
                    auto& target = m_render.viewport[op.viewportIndex];
                    if (target.depthMin != op.minZ || target.depthMax != op.maxZ)
                    {
                        target.depthMin = op.minZ;
                        target.depthMax = op.maxZ;

                        m_dirtyRenderStates.depthRangeDirtyPerVP |= (1UL << op.viewportIndex);
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ViewportDepthRanges;
                        m_passChangedRenderStates.depthRangeDirtyPerVP |= (1UL << op.viewportIndex);
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ViewportDepthRanges;
                    }
                }
            }

            void FrameExecutor::runSetScissorRect(const command::OpSetScissorRect& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                DEBUG_CHECK_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");
                if (op.viewportIndex < m_pass.viewportCount)
                {
                    auto& targetRect = m_render.viewport[op.viewportIndex].scissor;

                    const auto x = op.rect.left();
                    const auto y = (int)m_pass.height - op.rect.top() - op.rect.height();
                    //const auto y = op.rect.top();
                    const auto w = op.rect.width();
                    const auto h = op.rect.height();

                    if (x != targetRect[0] || y != targetRect[1] || w != targetRect[2] || h != targetRect[3])
                    {
                        targetRect[0] = x;
                        targetRect[1] = y;
                        targetRect[2] = w;
                        targetRect[3] = h;

                        m_dirtyRenderStates.scissorDirtyPerVP |= (1UL << op.viewportIndex);
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ScissorRects;
                        m_passChangedRenderStates.scissorDirtyPerVP |= (1UL << op.viewportIndex);
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ScissorRects;
                    }
                }
            }

            void FrameExecutor::runSetScissorState(const command::OpSetScissorState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.state != m_render.scissorEnabled)
                {
                    m_render.scissorEnabled = op.state;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::ScissorEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::ScissorEnabled;
                }
            }

            void FrameExecutor::runSetStencilReference(const command::OpSetStencilReference& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.front != m_render.stencil.front.ref)
                {
                    m_render.stencil.front.ref = op.front;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                }

                if (op.back != m_render.stencil.back.ref)
                {
                    m_render.stencil.back.ref = op.back;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                }
            }

            void FrameExecutor::runSetStencilWriteMask(const command::OpSetStencilWriteMask& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.front != m_render.stencil.front.writeMask)
                {
                    m_render.stencil.front.writeMask = op.front;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontWriteMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontWriteMask;
                }

                if (op.back != m_render.stencil.back.writeMask)
                {
                    m_render.stencil.back.writeMask = op.back;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackWriteMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackWriteMask;
                }
            }

            void FrameExecutor::runSetStencilCompareMask(const command::OpSetStencilCompareMask& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.front != m_render.stencil.front.compareMask)
                {
                    m_render.stencil.front.compareMask = op.front;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                }

                if (op.back != m_render.stencil.back.compareMask)
                {
                    m_render.stencil.back.compareMask = op.back;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                }
            }

            bool FrameExecutor::SetStencilFunc(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op)
            {
                auto value = TranslateCompareOp(op.compareOp);
                if (value == face.func && face.compareMask == op.compareMask && face.ref == op.referenceValue)
                    return false;

                face.func = value;
                face.compareMask = op.compareMask;
                face.ref = op.referenceValue;
                return true;
            }

            bool FrameExecutor::SetStencilWriteMask(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op)
            {
                if (face.writeMask == op.writeMask)
                    return false;

                face.writeMask = op.writeMask;
                return true;
            }

            bool FrameExecutor::SetStencilOps(RenderStates::ResolvedStencilFaceState& face, const StencilSideState& op)
            {
                auto failOp = TranslateStencilOp(op.failOp);
                auto depthFailOp = TranslateStencilOp(op.depthFailOp);
                auto passOp = TranslateStencilOp(op.passOp);

                if (failOp == face.failOp && depthFailOp == face.depthFailOp && passOp == face.passOp)
                    return false;

                face.failOp = failOp;
                face.depthFailOp = depthFailOp;
                face.passOp = passOp;
                return true;
            }

            void FrameExecutor::runSetStencilState(const command::OpSetStencilState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.state.enabled != m_render.stencil.enabled)
                {
                    m_render.stencil.enabled = op.state.enabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilEnabled;
                }

                if (m_render.stencil.enabled)
                {
                    if (SetStencilFunc(m_render.stencil.front, op.state.front))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontFuncReferenceMask;
                    }

                    if (SetStencilFunc(m_render.stencil.back, op.state.back))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackFuncReferenceMask;
                    }

                    if (SetStencilWriteMask(m_render.stencil.front, op.state.front))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontWriteMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontWriteMask;
                    }

                    if (SetStencilWriteMask(m_render.stencil.back, op.state.back))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackWriteMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackWriteMask;
                    }

                    if (SetStencilOps(m_render.stencil.front, op.state.front))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilFrontOps;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilFrontOps;
                    }

                    if (SetStencilOps(m_render.stencil.back, op.state.back))
                    {
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::StencilBackOps;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::StencilBackOps;
                    }                
                }
            }

            void FrameExecutor::runSetDepthBiasState(const command::OpSetDepthBiasState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                auto& val = m_render.depthBias;
                if (val.biasEnabled != op.state.enabled)
                {
                    val.biasEnabled = op.state.enabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthBiasEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthBiasEnabled;              
                }

                if (op.state.enabled)
                {
                    if (val.clamp != op.state.clamp || val.constant != op.state.constant || val.slope != op.state.slope)
                    {
                        val.clamp = op.state.clamp;
                        val.constant = op.state.constant;
                        val.slope = op.state.slope;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthBiasValues;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthBiasValues;
                    }
                }
            }

            static GLenum TranslateFillMode(PolygonMode mode)
            {
                auto ret = GL_FILL;
                switch (mode)
                {
                case PolygonMode::Line: ret = GL_LINE; break;
                case PolygonMode::Point: ret = GL_POINT; break;
                }
                return ret;
            }

            void FrameExecutor::runSetFillState(const command::OpSetFillState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                auto& val = m_render.polygon;
                auto mode = TranslateFillMode(op.state.mode);
                if (val.fill != mode)
                {
                    val.fill = mode;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::PolygonFillMode;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::PolygonFillMode;
                }

                if (val.lineWidth != op.state.lineWidth)
                {
                    val.lineWidth = op.state.lineWidth;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::PolygonLineWidth;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::PolygonLineWidth;
                }
            }

            void FrameExecutor::runSetPrimitiveAssemblyState(const command::OpSetPrimitiveAssemblyState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                const auto top = TranslateDrawTopology(op.state.topology);
                if (m_render.polygon.topology != top)
                {
                    m_render.polygon.topology = top;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::PolygonTopology;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::PolygonTopology;
                }

                if (m_render.polygon.restartEnabled != op.state.restartEnabled)
                {
                    m_render.polygon.restartEnabled = op.state.restartEnabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::PolygonPrimitiveRestart;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::PolygonPrimitiveRestart;
                }
            }

            static GLenum TranslateCullMode(CullMode mode)
            {
                switch (mode)
                {
                case CullMode::Back: return GL_BACK;
                case CullMode::Front: return GL_FRONT;
                case CullMode::Both: return GL_FRONT_AND_BACK;
                }
                return GL_NONE;
            }

            static GLenum TranslateFrontFace(FrontFace face)
            {
                return (face == FrontFace::CW) ? GL_CCW : GL_CW;
            }

            void FrameExecutor::runSetCullState(const command::OpSetCullState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                const auto mode = TranslateCullMode(op.state.mode);
                const auto face = TranslateFrontFace(op.state.face);
                const auto enabled = (mode != GL_NONE);

                if (m_render.cull.enabled != enabled)
                {
                    m_render.cull.enabled = enabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::CullEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::CullEnabled;
                }

                if (enabled)
                {
                    if (m_render.cull.cull != mode)
                    {
                        m_render.cull.cull = mode;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::CullMode;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::CullMode;
                    }
                }

                if (m_render.cull.front != face)
                {
                    m_render.cull.front = face;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::CullFrontFace;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::CullFrontFace;
                }
            }

            void FrameExecutor::runSetBlendColor(const command::OpSetBlendColor& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.color[0] != m_render.blendColor[0] || op.color[1] != m_render.blendColor[1] || op.color[2] != m_render.blendColor[2] || op.color[3] != m_render.blendColor[3])
                {
                    GL_PROTECT(glBlendColor(op.color[0], op.color[1], op.color[2], op.color[3]));
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::BlendingColor;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::BlendingColor;
                }
            }

            void FrameExecutor::updateBlendingEnable()
            {
                bool hasAnyBlending = false;
                for (uint32_t i = 0; i < m_pass.colorCount; ++i)
                {
                    if (m_render.rt[i].blend.enabled())
                    {
                        hasAnyBlending = true;
                        break;
                    }
                }

                if (hasAnyBlending != m_render.blendingEnabled)
                {
                    m_render.blendingEnabled = hasAnyBlending;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::BlendingEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::BlendingEnabled;
                }
            }

            void FrameExecutor::runSetBlendState(const command::OpSetBlendState& op)
            {
                DEBUG_CHECK_PASS_ONLY();
                DEBUG_CHECK_EX(op.rtIndex < m_pass.colorCount, "Render target was not enabled in pass");

                if (op.rtIndex < m_pass.colorCount)
                {
                    const auto colorOp = TranslateBlendOp(op.state.colorBlendOp);
                    const auto colorSrc = TranslateBlendFactor(op.state.srcColorBlendFactor);
                    const auto colorDest = TranslateBlendFactor(op.state.destColorBlendFactor);

                    const auto alphaOp = TranslateBlendOp(op.state.alphaBlendOp);
                    const auto alphaSrc = TranslateBlendFactor(op.state.srcAlphaBlendFactor);
                    const auto alphaDest = TranslateBlendFactor(op.state.destAlphaBlendFactor);

                    const auto rtMask = (1U << op.rtIndex);

                    auto& state = m_render.rt[op.rtIndex].blend;
                    if (colorOp != state.colorOp || alphaOp != state.alphaOp)
                    {
                        state.colorOp = colorOp;
                        state.alphaOp = alphaOp;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::BlendingEquation;
                        m_dirtyRenderStates.blendEquationDirtyPerRT |= rtMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::BlendingEquation;
                        m_passChangedRenderStates.blendEquationDirtyPerRT |= rtMask;
                    }

                    if (colorSrc != state.colorSrc || colorDest != state.colorDest || alphaSrc != state.alphaSrc || alphaDest != state.alphaDest)
                    {
                        state.colorSrc = colorSrc;
                        state.colorDest = colorDest;
                        state.alphaSrc = alphaSrc;
                        state.alphaDest = alphaDest;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::BlendingFunc;
                        m_dirtyRenderStates.blendFuncDirtyPerRT |= rtMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::BlendingFunc;
                        m_passChangedRenderStates.blendFuncDirtyPerRT |= rtMask;
                    }

                    updateBlendingEnable();
                }
            }

            void FrameExecutor::runSetDepthClipState(const command::OpSetDepthClipState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.state.enabled != m_render.depthClip.enabled)
                {
                    m_render.depthClip.enabled = op.state.enabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthBoundsEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthBoundsEnabled;
                }

                if (m_render.depthClip.enabled)
                {
                    if (m_render.depthClip.min != op.state.clipMin || m_render.depthClip.max != op.state.clipMax)
                    {
                        m_render.depthClip.min = op.state.clipMin;
                        m_render.depthClip.max = op.state.clipMax;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthBoundsRanges;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthBoundsRanges;
                    }
                }
            }

            void FrameExecutor::runSetDepthState(const command::OpSetDepthState& op)
            {
                DEBUG_CHECK_PASS_ONLY();

                if (op.state.enabled != m_render.depth.enabled)
                {
                    m_render.depth.enabled = op.state.enabled;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthEnabled;
                }

                if (m_render.depth.enabled)
                {
                    if (op.state.writeEnabled != m_render.depth.writeEnabled)
                    {
                        m_render.depth.writeEnabled = op.state.writeEnabled;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthWrite;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthWrite;
                    }

                    const auto func = TranslateCompareOp(op.state.depthCompareOp);
                    if (m_render.depth.func != func)
                    {
                        m_render.depth.func = func;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::DepthFunc;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::DepthFunc;
                    }
                }
            }

            void FrameExecutor::runSetMultisampleState(const command::OpSetMultisampleState& op)
            {
                if (m_render.multisample.alphaToCoverageEnabled != op.state.alphaToCoverageEnable)
                {
                    m_render.multisample.alphaToCoverageEnabled = op.state.alphaToCoverageEnable;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::AlphaCoverageEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::AlphaCoverageEnabled;
                }

                if (m_render.multisample.alphaCoverageDitherEnabled != op.state.alphaToCoverageDitherEnable)
                {
                    m_render.multisample.alphaCoverageDitherEnabled = op.state.alphaToCoverageDitherEnable;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::AlphaCoverageDitherEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::AlphaCoverageDitherEnabled;
                }            
            }

            void FrameExecutor::runSetColorMask(const command::OpSetColorMask& op)
            {
                DEBUG_CHECK_PASS_ONLY();
                DEBUG_CHECK_EX(op.rtIndex < m_pass.colorCount, "Render target was not enabled in pass");

                if (op.rtIndex < m_pass.colorCount)
                {
                    auto& colorMask = m_render.rt[op.rtIndex].colorMask;
                    if (colorMask != op.colorMask)
                    {
                        colorMask = op.colorMask;
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ColorMask;
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ColorMask;

                        const auto rtMask = (1U << op.rtIndex);
                        m_dirtyRenderStates.colorMaskDirtyPerRT |= rtMask;
                        m_passChangedRenderStates.colorMaskDirtyPerRT |= rtMask;
                    }
                }
            }

            //---

            void FrameExecutor::resetCommandBufferRenderState()
            {
                auto defaultStates = RenderStates();
                defaultStates.apply(m_passChangedRenderStates);
                m_passChangedRenderStates = RenderDirtyStateTrack();

                for (uint32_t i = 0; i < m_geometry.maxBoundVertexStreams; ++i)
                    GL_PROTECT(glBindVertexBuffer(i, 0, 0, 0));
                m_geometry.maxBoundVertexStreams = 0;
                m_geometry.vertexBindings.reset();
                m_geometry.indexBufferOffset = 0;
                m_geometry.vertexBindingsChanged = true;

                m_geometry.indexFormat = ImageFormat::UNKNOWN;
                m_geometry.indexStreamBinding = BufferView();
                m_geometry.indexBindingsChanged = true;

                m_params.paramBindings.reset();
                m_params.parameterBindingsChanged = true;

                m_objectBindings.reset();
            }

            void FrameExecutor::applyDirtyRenderStates()
            {
                if (m_dirtyRenderStates.flags.rawValue() != 0)
                {
                    m_render.apply(m_dirtyRenderStates);
                    m_dirtyRenderStates = RenderDirtyStateTrack();
                }
            }
       
            //---

            RuntimeBindingState::RuntimeBindingState()
            {
                memzero(m_currentImagesReadWriteMode, sizeof(m_currentImagesReadWriteMode));
                memzero(m_currentTextures, sizeof(m_currentTextures));
                memzero(m_currentSamplers, sizeof(m_currentSamplers));
            }

            void RuntimeBindingState::reset()
            {
                for (uint32_t i = 0; i < ARRAY_COUNT(m_currentUniformBuffers); ++i)
                    if (m_currentUniformBuffers[i])
                        GL_PROTECT(glBindBufferRange(GL_UNIFORM_BUFFER, i, 0, 0, 0));

                for (uint32_t i = 0; i < ARRAY_COUNT(m_currentStorageBuffers); ++i)
                    if (m_currentStorageBuffers[i])
                        GL_PROTECT(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, 0, 0, 0));

                for (uint32_t i = 0; i < ARRAY_COUNT(m_currentImages); ++i)
                    if (m_currentImages[i])
                        GL_PROTECT(glBindImageTexture(i, 0, 0, false, 0, GL_READ_ONLY, GL_R32F));

                for (uint32_t i = 0; i < ARRAY_COUNT(m_currentTextures); ++i)
                    if (m_currentTextures[i])
                        GL_PROTECT(glBindTextureUnit(i, 0));

                for (uint32_t i = 0; i < ARRAY_COUNT(m_currentSamplers); ++i)
                    if (m_currentSamplers[i])
                        GL_PROTECT(glBindSampler(i, 0));

                memzero(m_currentUniformBuffers, sizeof(m_currentUniformBuffers));
                memzero(m_currentStorageBuffers, sizeof(m_currentStorageBuffers));
                memzero(m_currentImages, sizeof(m_currentImages));
                memzero(m_currentImagesReadWriteMode, sizeof(m_currentImagesReadWriteMode));
                memzero(m_currentTextures, sizeof(m_currentTextures));
                memzero(m_currentSamplers, sizeof(m_currentSamplers));
            }

            void RuntimeBindingState::bindUniformBuffer(uint32_t slot, const ResolvedBufferView& buffer)
            {
                if (m_currentUniformBuffers[slot] != buffer)
                {
                    m_currentUniformBuffers[slot] = buffer;
                    GL_PROTECT(glBindBufferRange(GL_UNIFORM_BUFFER, slot, buffer.glBuffer, buffer.offset, buffer.size));
                }
            }

            void RuntimeBindingState::bindStorageBuffer(uint32_t slot, const ResolvedBufferView& buffer)
            {
                if (m_currentStorageBuffers[slot] != buffer)
                {
                    m_currentStorageBuffers[slot] = buffer;
                    GL_PROTECT(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, slot, buffer.glBuffer, buffer.offset, buffer.size));
                }
            }

            void RuntimeBindingState::bindImage(uint32_t slot, const ResolvedFormatedView& buffer, GLuint mode)
            {
                if (m_currentImages[slot] != buffer || m_currentImagesReadWriteMode[slot] != mode)
                {
                    m_currentImages[slot] = buffer;
                    m_currentImagesReadWriteMode[slot] = mode;
                    GL_PROTECT(glBindImageTexture(slot, buffer.glBufferView, 0, false, 0, mode, buffer.glViewFormat));
                }
            }

            void RuntimeBindingState::bindTexture(uint32_t slot, const ResolvedImageView& view)
            {
                if (m_currentTextures[slot] != view.glImageView)
                {
                    m_currentTextures[slot] = view.glImageView;
                    GL_PROTECT(glBindTextureUnit(slot, view.glImageView));
                }
            }

            void RuntimeBindingState::bindSampler(uint32_t slot, GLuint samplerView)
            {
                if (m_currentSamplers[slot] != samplerView)
                {
                    m_currentSamplers[slot] = samplerView;
                    GL_PROTECT(glBindSampler(slot, samplerView));
                }
            }

            //---

        } // exec
    } // gl4
} // rendering
