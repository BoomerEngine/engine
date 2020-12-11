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

        /// canvas renderer - hosts all necessary shaders and data to render canvas
        /// NOTE: this class must be externally synchronized as it's single thread only
        class RENDERING_CANVAS_API CanvasRenderer : public base::canvas::Canvas
        {
			RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderer, base::canvas::Canvas);

        public:
			struct Setup : public base::canvas::Canvas::Setup
			{
				RenderTargetView* backBufferColorRTV = nullptr;
				RenderTargetView* backBufferDepthRTV = nullptr;
				GraphicsPassLayoutObject* backBufferLayout = nullptr;
			};

			CanvasRenderer(const Setup& setup, const CanvasStorage* storage);
            ~CanvasRenderer();

			//--

			void finishPass();
			void startPass();

			//--

			/// extract generated command buffer chain (may be big if it includes rendering to 3d viewports)
			command::CommandBuffer* finishRecording();

			//--

		private:
			virtual void flushInternal(const base::canvas::Vertex* vertices, uint32_t numVertices, 
				const base::canvas::Attributes* attributes, uint32_t numAttributes,
				const void* data, uint32_t dataSize,
				const base::canvas::Batch* batches, uint32_t numBatches) override final;

			command::CommandWriter* m_commandBufferWriter;
			const CanvasStorage* m_storage;

			int m_currentAtlasIndex = -1;

			bool m_inPass = false;

			RenderTargetView* m_backBufferColorRTV = nullptr;
			RenderTargetView* m_backBufferDepthRTV = nullptr;
			GraphicsPassLayoutObject* m_backBufferLayout = nullptr;

			base::Matrix m_canvasToScreen;
        };

        //--

    } // canvas
} // rendering