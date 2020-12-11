/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/canvas/include/canvas.h"

namespace rendering
{
    namespace canvas
    {
		///---

		struct CanvasRenderStates
		{
			static const auto MAX_BLEND_OPS = (int)base::canvas::BlendOp::MAX;

			GraphicsRenderStatesObjectPtr m_mask = nullptr;
			GraphicsRenderStatesObjectPtr m_standardFill[MAX_BLEND_OPS];
			GraphicsRenderStatesObjectPtr m_maskedFill[MAX_BLEND_OPS];
		};

        ///---

        /// rendering handler for custom canvas batches
        class RENDERING_CANVAS_API ICanvasBatchRenderer : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasBatchRenderer);
			RTTI_DECLARE_POOL(POOL_CANVAS);

        public:
            virtual ~ICanvasBatchRenderer();

			//--

			/// prepare rendering for given pass layout
			virtual void prepareForLayout(const CanvasRenderStates& renderStates, GraphicsPassLayoutObject* pass) = 0;

            /// initialize handler, may allocate resources, load shaders, etc
            virtual bool initialize(CanvasStorage* owner, IDevice* drv) = 0;

			//--

			struct RenderData
			{
				uint32_t width = 0;
				uint32_t height = 0;

				const base::canvas::Vertex* vertices = nullptr;
				const void* customData = nullptr;

				const BufferObject* vertexBuffer = nullptr;
				const ImageSampledView* atlasImage = nullptr;
				const BufferStructuredView* atlasData = nullptr;
				const ImageSampledView* glyphImage = nullptr;
				base::canvas::BatchType batchType = base::canvas::BatchType::FillConvex;
				base::canvas::BlendOp blendOp = base::canvas::BlendOp::AlphaPremultiplied;
			};

            /// handle rendering of batches
			virtual void render(command::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const = 0;
        };

		//---

		class RENDERING_CANVAS_API ICanvasSimpleBatchRenderer : public ICanvasBatchRenderer
		{
			RTTI_DECLARE_VIRTUAL_CLASS(ICanvasSimpleBatchRenderer, ICanvasBatchRenderer);

		public:
			ICanvasSimpleBatchRenderer();
			virtual ~ICanvasSimpleBatchRenderer();

			// load the default draw shader
			virtual ShaderFilePtr loadMainShaderFile() = 0;

		protected:
			virtual bool initialize(CanvasStorage* owner, IDevice* drv) override final;
			virtual void prepareForLayout(const CanvasRenderStates& renderStates, GraphicsPassLayoutObject* pass) override final;
			virtual void render(command::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override;

			static const auto MAX_BLEND_OPS = (int)base::canvas::BlendOp::MAX;

			struct ShaderGroup
			{
				uint64_t layoutKey = 0;

				GraphicsPipelineObjectPtr m_mask;
				GraphicsPipelineObjectPtr m_standardFill[MAX_BLEND_OPS];
				GraphicsPipelineObjectPtr m_maskedFill[MAX_BLEND_OPS];

				const GraphicsPipelineObject* selectShader(command::CommandWriter& cmd, const RenderData& data) const;
			};

			const ShaderGroup* m_activeShaderGroup = nullptr;
			base::InplaceArray<ShaderGroup, 2> m_shaderGroups;

			ShaderObjectPtr m_shaderFill;
			ShaderObjectPtr m_shaderMask;
		};

        //---

        /// get the static handler ID
        template< typename T >
        static uint16_t GetHandlerIndex()
        {
            static const auto id = T::GetStaticClass()->userIndex();
            DEBUG_CHECK_EX(id != INDEX_NONE, "Handled class not properly registered");
            return (uint16_t)id;
        }

        //--

    } // canvas
} // rendering