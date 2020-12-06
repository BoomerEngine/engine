/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

namespace rendering
{
    namespace api
    {
        namespace gl4
        {

			//---

			static const uint32_t MAX_VIEWPORTS = 16;
			static const uint32_t MAX_TARGETS = 8;

/*			ViewportRect_First,
				ViewportRect_Last = ViewportRect_First + MAX_VIEWPORTS - 1,

				ScissorRect_First,
				ScissorRect_Last = ScissorRect_First + MAX_VIEWPORTS - 1,*/

			enum StateBit
			{
				State_ScissorEnabled,

				State_BlendEnabled,
				State_BlendConstColor,
				State_BlendEquation_First,
				State_BlendEquation_Last = State_BlendEquation_First + MAX_TARGETS - 1,

				State_ColorMask_First,
				State_ColorMask_Last = State_ColorMask_First + MAX_TARGETS - 1,

				State_DepthEnabled,
				State_DepthFunc,
				State_DepthBoundsEnabled,
				State_DepthBoundsRanges,
				State_DepthWriteEnabled,
				State_DepthBiasEnabled,
				State_DepthBiasValues,

				State_StencilEnabled,
				State_StencilFrontFuncReferenceMask,
				State_StencilBackFuncReferenceMask,
				State_StencilFrontWriteMask,
				State_StencilBackWriteMask,
				State_StencilFrontOps,
				State_StencilBackOps,
				
				State_PolygonFillMode,
				State_PolygonLineWidth,
				State_PolygonPrimitiveRestart,
				State_PolygonTopology,

				State_CullEnabled,
				State_CullMode,
				State_CullFrontFace,

				State_AlphaCoverageEnabled,
				State_AlphaCoverageDitherEnabled,
				
				State_MAX,
			};

			static_assert(State_MAX < 64, "To many stats");

			//---

			// mask of stats
			struct StateMask
			{
				INLINE StateMask() {}
				INLINE StateMask(const StateMask& other) = default;
				INLINE StateMask& operator=(const StateMask& other) = default;

				ALWAYS_INLINE static uint64_t bitMask(StateBit bit)
				{
					return 1ULL << (int)bit;
				}

				INLINE bool operator&(StateBit bit) const
				{
					return (word & bitMask(bit)) != 0;
				}

				INLINE StateMask& operator|=(StateBit bit)
				{
					word |= bitMask(bit);
					return *this;
				}

				INLINE StateMask& operator-=(StateBit bit)
				{
					word &= ~bitMask(bit);
					return *this;
				}

				INLINE StateMask& operator|=(const StateMask& other)
				{
					word |= other.word;
					return *this;
				}

				INLINE StateMask& operator-=(const StateMask& other)
				{
					word &= ~other.word;
					return *this;
				}

				uint64_t word = 0;
			};

			//---

			// resolved stat values
#pragma pack(push)
#pragma pack(1)
			struct StateValues : public base::NoCopy
			{
				struct Blend
				{
					uint16_t colorOp = GL_FUNC_ADD;
					uint16_t colorSrc = GL_ONE;
					uint16_t colorDest = GL_ZERO;
					uint16_t alphaOp = GL_FUNC_ADD;
					uint16_t alphaSrc = GL_ONE;
					uint16_t alphaDest = GL_ZERO;

					ALWAYS_INLINE bool operator!=(const Blend& other) const
					{
						return 0 != memcmp(this, &other, sizeof(Blend));
					}
				};

				struct StencilFace
				{
					uint16_t func = GL_ALWAYS;
					uint16_t failOp = GL_KEEP;
					uint16_t depthFailOp = GL_KEEP;
					uint16_t passOp = GL_KEEP;
					uint8_t ref = 0;
					uint8_t compareMask = 0xFF;
					uint8_t writeMask = 0xFF;
				};

				struct Stencil
				{
					StencilFace front;
					StencilFace back;
					bool enabled = false;
				};

				struct Depth
				{
					bool enabled = false;
					bool writeEnabled = true;
					bool clipEnabled = false;
					bool biasEnabled = false;
					uint16_t func = GL_ALWAYS;
					float clipMin = 0.0f;
					float clipMax = 1.0f;
					float biasConstant = 0.0f;
					float biasSlope = 0.0f;
					float biasClamp = 0.0f;
				};

				struct Polygon
				{
					uint16_t fill = GL_FILL;
					uint16_t topology = GL_TRIANGLES;
					float lineWidth = 1.0f;
					bool restartEnabled = false;
				};

				struct Cull
				{
					uint16_t front = GL_CW;
					uint16_t cull = GL_BACK;
					bool enabled = false;
				};

				struct Multisample
				{
					bool alphaToCoverageEnabled = false;
					bool alphaToOneEnabled = false;
					bool alphaCoverageDitherEnabled = false;
				};

				struct Common
				{
					bool blendingEnabled = false;
					bool scissorEnabled = false;
					float blendColor[4] = { 1,1,1,1 };
				};

				Depth depth;
				Cull cull; 
				Stencil stencil;
				Multisample multisample;
				Common common;
				Polygon polygon;
				Blend blend[MAX_TARGETS];
				uint8_t colorMask[MAX_TARGETS];

				//--

				static const StateValues& DEFAULT_STATES();

				StateValues();
				StateValues& operator=(const StateValues& other);

				void apply(StateMask mask) const;

				void apply(const StateValues& base, StateMask mask, StateMask& outChanged);

				void apply(const GraphicsRenderStatesSetup& setup, StateMask& outChanged);
			};
#pragma pack(pop)

            //---
			
			class StateResources : public base::NoCopy
			{
			public:
				StateResources();

				void bindUniformBuffer(uint32_t slot, const ResolvedBufferView& view);
				void bindStorageBuffer(uint32_t slot, const ResolvedBufferView& view);
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

        } // gl4
    } // api
} // rendering

