/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasDefaultBatchRenderer.h"
#include "rendering/device/include/renderingPipeline.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingBuffer.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"

namespace rendering
{
    namespace canvas
    {
		//---

        //---


		CanvasDefaultBatchRenderer::CanvasDefaultBatchRenderer()
			: m_activeShaderGroup(nullptr)
		{
		}

		CanvasDefaultBatchRenderer::~CanvasDefaultBatchRenderer()
		{
		}

		void CanvasDefaultBatchRenderer::prepareForLayout(const CanvasRenderStates& renderStates, GraphicsPassLayoutObject* pass)
		{
			
		}

		bool CanvasDefaultBatchRenderer::initialize(CanvasStorage* owner, IDevice* drv)
		{
			if (auto shader = resCanvasShaderFill.loadAndGet())
				m_shaderFill = shader->rootShader()->deviceShader();

			if (auto shader = resCanvasShaderMask.loadAndGet())
				m_shaderMask = shader->rootShader()->deviceShader();

			return m_shaderFill && m_shaderMask;
		}


        //---

    } // canvas
} // rendering