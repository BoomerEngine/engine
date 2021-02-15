/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#include "build.h"
#include "gl4StateCache.h"
#include "gl4Buffer.h"
#include "gl4Image.h"

namespace rendering
{
    namespace api
    {
        namespace gl4
        {
			//--

			static GLenum TranslateCompareOp(int op)
			{
				switch ((CompareOp)op)
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

			static GLenum TranslateBlendFactor(int op)
			{
				switch ((BlendFactor)op)
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

			static GLenum TranslateBlendOp(int op)
			{
				switch ((BlendOp)op)
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

			static GLenum TranslateStencilOp(int op)
			{
				switch ((StencilOp)op)
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

			static GLenum TranslateLogicOp(int op)
			{
				switch ((LogicalOp)op)
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

			static GLenum TranslateDrawTopology(int topology)
			{
				switch ((PrimitiveTopology)topology)
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

			static GLenum TranslateFillMode(int mode)
			{
				auto ret = GL_FILL;
				switch ((FillMode)mode)
				{
				case FillMode::Line: ret = GL_LINE; break;
				case FillMode::Point: ret = GL_POINT; break;
				}
				return ret;
			}

			static GLenum TranslateCullMode(int mode)
			{
				switch ((CullMode)mode)
				{
				case CullMode::Back: return GL_BACK;
				case CullMode::Front: return GL_FRONT;
				case CullMode::Both: return GL_FRONT_AND_BACK;
				}
				return GL_NONE;
			}

			static GLenum TranslateFrontFace(int face)
			{
				return ((FrontFace)face == FrontFace::CW) ? GL_CCW : GL_CW;
			}

			//--

			static StateValues GDefaultStates;

			const StateValues& StateValues::DEFAULT_STATES()
			{
				return GDefaultStates;
			}

			static void glToggle(GLenum stat, bool val)
			{
				if (val)
					glEnable(stat);
				else
					glDisable(stat);
			}

			StateValues::StateValues()
			{
				memset(colorMask, 0x0F, sizeof(colorMask));
			}

			StateValues& StateValues::operator=(const StateValues& other)
			{
				memcpy(this, &other, sizeof(StateValues));
				return *this;
			}
			
			void StateValues::apply(StateMask bits) const
			{
				auto mask = bits.word;
				while (mask)
				{
					auto stateBit = __builtin_ctzll(mask);
					mask ^= mask & -mask;

					switch (stateBit)
					{
						case State_ScissorEnabled:
							GL_PROTECT(glToggle(GL_SCISSOR_TEST, common.scissorEnabled));
							break;

						case State_BlendEnabled:
							GL_PROTECT(glToggle(GL_BLEND, common.blendingEnabled));
							break;

						case State_BlendConstColor:
							GL_PROTECT(glBlendColor(common.blendColor[0], common.blendColor[1], common.blendColor[2], common.blendColor[3]));
							break;

						case State_BlendEquation_First + 0:
						case State_BlendEquation_First + 1:
						case State_BlendEquation_First + 2:
						case State_BlendEquation_First + 3:
						case State_BlendEquation_First + 4:
						case State_BlendEquation_First + 5:
						case State_BlendEquation_First + 6:
						case State_BlendEquation_First + 7:
						{
							const int index = stateBit - State_BlendEquation_First;
							const auto& state = blend[index];
							GL_PROTECT(glBlendEquationSeparatei(index, state.colorOp, state.alphaOp));
							GL_PROTECT(glBlendFuncSeparatei(index, state.colorSrc, state.colorDest, state.alphaSrc, state.alphaDest));
							break;
						}

						case State_ColorMask_First + 0:
						case State_ColorMask_First + 1:
						case State_ColorMask_First + 2:
						case State_ColorMask_First + 3:
						case State_ColorMask_First + 4:
						case State_ColorMask_First + 5:
						case State_ColorMask_First + 6:
						case State_ColorMask_First + 7:
						{
							const int index = stateBit - State_ColorMask_First;
							const auto mask = colorMask[index];
							GL_PROTECT(glColorMaski(index, mask & 1, mask & 2, mask & 4, mask & 8));
							break;
						}

						case State_DepthEnabled:
							GL_PROTECT(glToggle(GL_DEPTH_TEST, depth.enabled));
							break;

						case State_DepthWriteEnabled:
							GL_PROTECT(glDepthMask(depth.writeEnabled));
							break;

						case State_DepthFunc:
							GL_PROTECT(glDepthFunc(depth.func));
							break;

						case State_DepthBiasEnabled:
							GL_PROTECT(glToggle(GL_POLYGON_OFFSET_FILL, depth.biasEnabled));
							GL_PROTECT(glToggle(GL_POLYGON_OFFSET_LINE, depth.biasEnabled));
							break;

						case State_DepthBiasValues:
							GL_PROTECT(glPolygonOffsetClamp(depth.biasSlope, depth.biasConstant, depth.biasClamp));
							break;

						case State_StencilFrontFuncReferenceMask:
							GL_PROTECT(glStencilFuncSeparate(GL_FRONT, stencil.front.func, stencil.front.ref, stencil.front.compareMask));
							break;

						case State_StencilBackFuncReferenceMask:
							GL_PROTECT(glStencilFuncSeparate(GL_BACK, stencil.back.func, stencil.back.ref, stencil.back.compareMask));
							break;

						case State_StencilFrontOps:
							GL_PROTECT(glStencilOpSeparate(GL_FRONT, stencil.front.failOp, stencil.front.depthFailOp, stencil.front.passOp));
							break;

						case State_StencilBackOps:
							GL_PROTECT(glStencilOpSeparate(GL_BACK, stencil.back.failOp, stencil.back.depthFailOp, stencil.back.passOp));
							break;

						case State_StencilFrontWriteMask:
							GL_PROTECT(glStencilMaskSeparate(GL_FRONT, stencil.front.writeMask));
							break;

						case State_StencilBackWriteMask:
							GL_PROTECT(glStencilMaskSeparate(GL_BACK, stencil.back.writeMask));
							break;

						case State_StencilEnabled:
							GL_PROTECT(glToggle(GL_STENCIL_TEST, stencil.enabled));
							break;

						case State_PolygonFillMode:
							GL_PROTECT(glPolygonMode(GL_FRONT_AND_BACK, polygon.fill));
							break;

						case State_PolygonLineWidth:
							GL_PROTECT(glLineWidth(polygon.lineWidth));
							break;

						case State_PolygonPrimitiveRestart:
							GL_PROTECT(glToggle(GL_PRIMITIVE_RESTART_FIXED_INDEX, polygon.restartEnabled));
							break;

						case State_PolygonTopology:
							GL_PROTECT(glToggle(GL_PROGRAM_POINT_SIZE, polygon.topology == GL_POINTS));
							break;

						case State_CullFrontFace:
							GL_PROTECT(glFrontFace(cull.front));
							break;

						case State_CullEnabled:
						case State_CullMode:
							if (cull.cull == GL_NONE)
							{
								GL_PROTECT(glToggle(GL_CULL_FACE, false));
							}
							else
							{
								GL_PROTECT(glToggle(GL_CULL_FACE, cull.enabled));
								GL_PROTECT(glCullFace(cull.cull));
							}
							break;

						case State_DepthBoundsEnabled:
							GL_PROTECT(glToggle(GL_DEPTH_CLAMP, depth.clipEnabled));
							break;

						case State_DepthBoundsRanges:
							GL_PROTECT(glDepthBoundsEXT(depth.clipMin, depth.clipMax));
							break;

						case State_AlphaCoverageEnabled:
							GL_PROTECT(glToggle(GL_SAMPLE_ALPHA_TO_COVERAGE, multisample.alphaToCoverageEnabled));
							break;

						case State_AlphaCoverageDitherEnabled:
							GL_PROTECT(glAlphaToCoverageDitherControlNV(multisample.alphaCoverageDitherEnabled ? GL_ALPHA_TO_COVERAGE_DITHER_ENABLE_NV : GL_ALPHA_TO_COVERAGE_DITHER_DISABLE_NV));
							break;

						default:
							ASSERT(!"Unknown state parameter");
					}
				}
			}

			void StateValues::applyMasked(const StateValues& base, StateMask bits, StateMask& changed)
			{
				auto mask = bits.word;
				while (mask)
				{
					auto stateBit = (StateBit) __builtin_ctzll(mask);
					mask ^= mask & -mask;

					switch ((int)stateBit)
					{
					case State_ScissorEnabled:
						if (common.scissorEnabled != base.common.scissorEnabled)
						{
							common.scissorEnabled = base.common.scissorEnabled;
							changed |= stateBit;
						}
						break;

					case State_BlendEnabled:
						if (common.blendingEnabled != base.common.blendingEnabled)
						{
							common.blendingEnabled = base.common.blendingEnabled;
							changed |= stateBit;
						}
						break;

					case State_BlendConstColor:
						common.blendColor[0] = base.common.blendColor[0];
						common.blendColor[1] = base.common.blendColor[1];
						common.blendColor[2] = base.common.blendColor[2];
						common.blendColor[3] = base.common.blendColor[3];
						changed |= stateBit;
						break;

					case State_BlendEquation_First + 0:
					case State_BlendEquation_First + 1:
					case State_BlendEquation_First + 2:
					case State_BlendEquation_First + 3:
					case State_BlendEquation_First + 4:
					case State_BlendEquation_First + 5:
					case State_BlendEquation_First + 6:
					case State_BlendEquation_First + 7:
					{
						const int index = stateBit - State_BlendEquation_First;
						if (blend[index] != base.blend[index])
						{
							blend[index] = base.blend[index];
							changed |= stateBit;
						}
						break;
					}

					case State_ColorMask_First + 0:
					case State_ColorMask_First + 1:
					case State_ColorMask_First + 2:
					case State_ColorMask_First + 3:
					case State_ColorMask_First + 4:
					case State_ColorMask_First + 5:
					case State_ColorMask_First + 6:
					case State_ColorMask_First + 7:
					{
						const int index = stateBit - State_ColorMask_First;
						if (base.colorMask[index] != colorMask[index])
						{
							colorMask[index] = base.colorMask[index];
							changed |= stateBit;
						}
						break;
					}

					case State_DepthEnabled:
						if (depth.enabled != base.depth.enabled)
						{
							depth.enabled = base.depth.enabled;
							changed |= stateBit;
						}
						break;

					case State_DepthWriteEnabled:
						if (depth.writeEnabled != base.depth.writeEnabled)
						{
							depth.writeEnabled = base.depth.writeEnabled;
							changed |= stateBit;
						}
						break;

					case State_DepthFunc:
						if (depth.func != base.depth.func)
						{
							depth.func = base.depth.func;
							changed |= stateBit;
						}
						break;

					case State_DepthBiasEnabled:
						if (depth.biasEnabled != base.depth.biasEnabled)
						{
							depth.biasEnabled = base.depth.biasEnabled;
							changed |= stateBit;
						}
						break;

					case State_DepthBiasValues:
						depth.biasSlope = base.depth.biasSlope;
						depth.biasConstant = base.depth.biasConstant;
						depth.biasClamp = base.depth.biasClamp;
						changed |= stateBit;
						break;

					case State_StencilFrontFuncReferenceMask:
						if (stencil.front.func != base.stencil.front.func || stencil.front.ref != base.stencil.front.ref || stencil.front.compareMask != base.stencil.front.compareMask)
						{
							stencil.front.func = base.stencil.front.func;
							//stencil.front.ref = base.stencil.front.ref;
							stencil.front.compareMask = base.stencil.front.compareMask;
							changed |= stateBit;
						}
						break;

					case State_StencilBackFuncReferenceMask:
						if (stencil.back.func != base.stencil.back.func || stencil.back.ref != base.stencil.back.ref || stencil.back.compareMask != base.stencil.back.compareMask)
						{
							stencil.back.func = base.stencil.back.func;
							//stencil.back.ref = base.stencil.back.ref;
							stencil.back.compareMask = base.stencil.back.compareMask;
							changed |= stateBit;
						}
						break;

					case State_StencilFrontOps:
						if (stencil.front.failOp != base.stencil.front.failOp || stencil.front.depthFailOp != base.stencil.front.depthFailOp || stencil.front.passOp != base.stencil.front.passOp)
						{
							stencil.front.failOp = base.stencil.front.failOp;
							stencil.front.depthFailOp = base.stencil.front.depthFailOp;
							stencil.front.passOp = base.stencil.front.passOp;
							changed |= stateBit;
						}
						break;

					case State_StencilBackOps:
						if (stencil.back.failOp != base.stencil.back.failOp || stencil.back.depthFailOp != base.stencil.back.depthFailOp || stencil.back.passOp != base.stencil.back.passOp)
						{
							stencil.back.failOp = base.stencil.back.failOp;
							stencil.back.depthFailOp = base.stencil.back.depthFailOp;
							stencil.back.passOp = base.stencil.back.passOp;
							changed |= stateBit;
						}
						break;

					case State_StencilFrontWriteMask:
						if (stencil.front.writeMask != base.stencil.front.writeMask)
						{
							stencil.front.writeMask = base.stencil.front.writeMask;
							changed |= stateBit;
						}
						break;

					case State_StencilBackWriteMask:
						if (stencil.back.writeMask != base.stencil.back.writeMask)
						{
							stencil.back.writeMask = base.stencil.back.writeMask;
							changed |= stateBit;
						}
						break;

					case State_StencilEnabled:
						if (stencil.enabled != base.stencil.enabled)
						{
							stencil.enabled = base.stencil.enabled;
							changed |= stateBit;
						}
						break;

					case State_PolygonFillMode:
						if (polygon.fill != base.polygon.fill)
						{
							polygon.fill = base.polygon.fill;
							changed |= stateBit;
						}
						break;

					case State_PolygonLineWidth:
						if (polygon.lineWidth != base.polygon.lineWidth)
						{
							polygon.lineWidth = base.polygon.lineWidth;
							changed |= stateBit;
						}
						break;

					case State_PolygonPrimitiveRestart:
						if (polygon.restartEnabled != base.polygon.restartEnabled)
						{
							polygon.restartEnabled = base.polygon.restartEnabled;
							changed |= stateBit;
						}
						break;

					case State_PolygonTopology:
						if (polygon.topology != base.polygon.topology)
						{
							polygon.topology = base.polygon.topology;
							changed |= stateBit;
						}
						break;

					case State_CullFrontFace:
						if (cull.front != base.cull.front)
						{
							cull.front = base.cull.front;
							changed |= stateBit;
						}
						break;

					case State_CullMode:
						if (cull.cull != base.cull.cull)
						{
							cull.cull = base.cull.cull;
							changed |= stateBit;
						}
						break;

					case State_CullEnabled:
						if (cull.enabled != base.cull.enabled)
						{
							cull.enabled = base.cull.enabled;
							changed |= stateBit;
						}
						break;

					case State_DepthBoundsEnabled:
						if (depth.clipEnabled != base.depth.clipEnabled)
						{
							depth.clipEnabled = base.depth.clipEnabled;
							changed |= stateBit;
						}
						break;

					case State_DepthBoundsRanges:
						depth.clipMin = base.depth.clipMin;
						depth.clipMax = base.depth.clipMax;
						changed |= stateBit;
						break;

					case State_AlphaCoverageEnabled:
						if (multisample.alphaToCoverageEnabled != base.multisample.alphaToCoverageEnabled)
						{
							multisample.alphaToCoverageEnabled = base.multisample.alphaToCoverageEnabled;
							changed |= stateBit;
						}
						break;

					case State_AlphaCoverageDitherEnabled:
						if (multisample.alphaCoverageDitherEnabled != base.multisample.alphaCoverageDitherEnabled)
						{
							multisample.alphaCoverageDitherEnabled = base.multisample.alphaCoverageDitherEnabled;
							changed |= stateBit;
						}
						break;

					default:
						ASSERT(!"Unknown state parameter");
					}
				}
			}

			//--

			template< typename T, typename W >
			ALWAYS_INLINE static bool Apply(T& currentValue, W newValue)
			{
				if (currentValue != (T)(newValue))
				{
					currentValue = (T)(newValue);
					return true;
				}

				return false;
			}

			void StateValues::apply(const GraphicsRenderStatesSetup& setup, StateMask& outChanged)
			{
				PC_SCOPE_LVL1(ApplyGraphicsStates);

				if (setup.mask.test(GraphicRenderStatesBit::FillMode))
				{
					polygon.fill = TranslateFillMode(setup.common.fillMode);
					outChanged |= State_PolygonFillMode;
				}

				if (setup.mask.test(GraphicRenderStatesBit::BlendingEnabled))
				{
					common.blendingEnabled = setup.common.blendingEnabled;
					outChanged |= State_BlendEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::ScissorEnabled))
				{
					common.scissorEnabled = setup.common.scissorEnabled;
					outChanged |= State_ScissorEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::CullEnabled))
				{
					cull.enabled = setup.common.cullEnabled;
					outChanged |= State_CullEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::CullFrontFace))
				{
					cull.front = TranslateFrontFace(setup.common.cullFrontFace);
					outChanged |= State_CullFrontFace;
				}

				if (setup.mask.test(GraphicRenderStatesBit::CullMode))
				{
					cull.cull = TranslateCullMode(setup.common.cullMode);
					outChanged |= State_CullMode;
				}

				if (setup.mask.test(GraphicRenderStatesBit::DepthEnabled))
				{
					depth.enabled = setup.common.depthEnabled;
					outChanged |= State_DepthEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::DepthWriteEnabled))
				{
					depth.writeEnabled = setup.common.depthWriteEnabled;
					outChanged |= State_DepthWriteEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::DepthFunc))
				{
					depth.func = TranslateCompareOp(setup.common.depthCompareOp);
					outChanged |= State_DepthFunc;
				}

				if (setup.mask.test(GraphicRenderStatesBit::DepthBiasEnabled))
				{
					depth.biasEnabled = setup.common.depthBiasEnabled;
					outChanged |= State_DepthBiasEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::DepthBiasValue))
				{
					depth.biasConstant = setup.depthStencilStates.depthBias;
					depth.biasSlope = setup.depthStencilStates.depthBiasSlope;
					depth.biasClamp = setup.depthStencilStates.depthBiasClamp;
					outChanged |= State_DepthBiasValues;
				}

				/*if (setup.mask.test(GraphicRenderStatesBit::DepthWriteEnabled))
					if (Apply(depth.biasEnabled, (CompareOp)setup.common.depthBiasEnabled))
						outChanged |= State_DepthBiasEnabled;*/

				if (setup.mask.test(GraphicRenderStatesBit::StencilEnabled))
				{
					stencil.enabled = setup.common.stencilEnabled;
					outChanged |= State_StencilEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilFrontDepthFailOp) ||
					setup.mask.test(GraphicRenderStatesBit::StencilFrontPassOp) || 
					setup.mask.test(GraphicRenderStatesBit::StencilFrontFailOp))
				{
					stencil.front.failOp = TranslateStencilOp(setup.common.stencilFrontFailOp);
					stencil.front.passOp = TranslateStencilOp(setup.common.stencilFrontPassOp);
					stencil.front.depthFailOp = TranslateStencilOp(setup.common.stencilFrontDepthFailOp);
					outChanged |= State_StencilFrontOps;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilReadMask) ||
					setup.mask.test(GraphicRenderStatesBit::StencilFrontFunc))
				{
					stencil.front.func = TranslateCompareOp(setup.common.stencilFrontCompareOp);
					stencil.front.compareMask = setup.depthStencilStates.stencilReadMask;
					outChanged |= State_StencilFrontFuncReferenceMask;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilWriteMask))
				{
					stencil.front.writeMask = setup.depthStencilStates.stencilWriteMask;
					outChanged |= State_StencilFrontWriteMask;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilBackFailOp) ||
					setup.mask.test(GraphicRenderStatesBit::StencilBackPassOp) ||
					setup.mask.test(GraphicRenderStatesBit::StencilBackDepthFailOp))
				{
					stencil.back.failOp = TranslateStencilOp(setup.common.stencilBackFailOp);
					stencil.back.passOp = TranslateStencilOp(setup.common.stencilBackPassOp);
					stencil.back.depthFailOp = TranslateStencilOp(setup.common.stencilBackDepthFailOp);
					outChanged |= State_StencilBackOps;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilBackFunc) || 
					setup.mask.test(GraphicRenderStatesBit::StencilReadMask))
				{
					stencil.back.func = TranslateCompareOp(setup.common.stencilBackCompareOp);
					stencil.back.compareMask = setup.depthStencilStates.stencilReadMask;
					outChanged |= State_StencilBackFuncReferenceMask;
				}

				if (setup.mask.test(GraphicRenderStatesBit::StencilWriteMask))
				{
					stencil.back.writeMask = setup.depthStencilStates.stencilWriteMask;
					outChanged |= State_StencilBackWriteMask;
				}

				if (setup.mask.test(GraphicRenderStatesBit::PrimitiveRestartEnabled))
				{
					polygon.restartEnabled = setup.common.primitiveRestartEnabled;
					outChanged |= State_PolygonPrimitiveRestart;
				}

				if (setup.mask.test(GraphicRenderStatesBit::PrimitiveTopology))
				{
					polygon.topology = TranslateDrawTopology(setup.common.primitiveTopology);
					outChanged |= State_PolygonTopology;
				}

				if (setup.mask.test(GraphicRenderStatesBit::AlphaCoverageEnabled))
				{
					multisample.alphaToCoverageEnabled = setup.common.alphaToCoverageEnable;
					outChanged |= State_AlphaCoverageEnabled;
				}

				if (setup.mask.test(GraphicRenderStatesBit::AlphaCoverageDitherEnabled))
				{
					multisample.alphaCoverageDitherEnabled = setup.common.alphaToCoverageDitherEnable;
					outChanged |= State_AlphaCoverageDitherEnabled;
				}

				for (uint32_t i = 0; i < 8; ++i)
				{
					if (setup.mask.test(GraphicsRenderStatesSetup::COLOR_MASK_DIRTY_BITS[i]))
					{
						colorMask[i] = setup.colorMasks[i];
						outChanged |= (StateBit)(State_ColorMask_First + i);
					}

					if (setup.mask.test(GraphicsRenderStatesSetup::BLEND_COLOR_OP_DIRTY_BITS[i]) ||
						setup.mask.test(GraphicsRenderStatesSetup::BLEND_ALPHA_OP_DIRTY_BITS[i]) ||
						setup.mask.test(GraphicsRenderStatesSetup::BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[i]) || 
						setup.mask.test(GraphicsRenderStatesSetup::BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[i]) ||
						setup.mask.test(GraphicsRenderStatesSetup::BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[i]) ||
						setup.mask.test(GraphicsRenderStatesSetup::BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[i]))
					{
						blend[i].colorOp = TranslateBlendOp(setup.blendStates[i].colorBlendOp);
						blend[i].colorSrc = TranslateBlendFactor(setup.blendStates[i].srcColorBlendFactor);
						blend[i].colorDest = TranslateBlendFactor(setup.blendStates[i].destColorBlendFactor);
						blend[i].alphaOp = TranslateBlendOp(setup.blendStates[i].alphaBlendOp);
						blend[i].alphaSrc = TranslateBlendFactor(setup.blendStates[i].srcAlphaBlendFactor);
						blend[i].alphaDest = TranslateBlendFactor(setup.blendStates[i].destAlphaBlendFactor);
						outChanged |= (StateBit)(State_BlendEquation_First + i);
					}
				}
			}

			//--

			StateResources::StateResources()
			{
				memzero(m_currentImagesReadWriteMode, sizeof(m_currentImagesReadWriteMode));
				memzero(m_currentTextures, sizeof(m_currentTextures));
				memzero(m_currentSamplers, sizeof(m_currentSamplers));
			}

			void StateResources::reset()
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

			void StateResources::bindUniformBuffer(uint32_t slot, const ResolvedBufferView& view)
			{
				if (m_currentUniformBuffers[slot] != view)
				{
					m_currentUniformBuffers[slot] = view;
					GL_PROTECT(glBindBufferRange(GL_UNIFORM_BUFFER, slot, view.glBuffer, view.offset, view.size));
				}
			}

			void StateResources::bindStorageBuffer(uint32_t slot, const ResolvedBufferView& view)
			{
				if (m_currentStorageBuffers[slot] != view)
				{
					m_currentStorageBuffers[slot] = view;
					GL_PROTECT(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, slot, view.glBuffer, view.offset, view.size));
				}
			}

			void StateResources::bindImage(uint32_t slot, const ResolvedFormatedView& buffer, GLuint mode)
			{
				if (m_currentImages[slot] != buffer || m_currentImagesReadWriteMode[slot] != mode)
				{
					m_currentImages[slot] = buffer;
					m_currentImagesReadWriteMode[slot] = mode;
					GL_PROTECT(glBindImageTexture(slot, buffer.glBufferView, 0, false, 0, mode, buffer.glViewFormat));
				}
			}

			void StateResources::bindTexture(uint32_t slot, const ResolvedImageView& view)
			{
				if (m_currentTextures[slot] != view.glImageView)
				{
					m_currentTextures[slot] = view.glImageView;
					GL_PROTECT(glBindTextureUnit(slot, view.glImageView));
				}
			}

			void StateResources::bindSampler(uint32_t slot, GLuint samplerView)
			{
				if (m_currentSamplers[slot] != samplerView)
				{
					m_currentSamplers[slot] = samplerView;
					GL_PROTECT(glBindSampler(slot, samplerView));
				}
			}

			//--

        } // exec
    } // gl4
} // rendering
