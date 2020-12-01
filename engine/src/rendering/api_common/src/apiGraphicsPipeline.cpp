/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "apiGraphicsPipeline.h"
#include "apiGraphicsPassLayout.h"
#include "apiGraphicsRenderStates.h"
#include "apiShaders.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseGraphicsPipeline::IBaseGraphicsPipeline(IBaseThread* owner, const IBaseShaders* shaders, const IBaseGraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates)
			: IBaseObject(owner, ObjectType::GraphicsPipelineObject)
			, m_passLayout(passLayout)
			, m_mergedRenderStates(mergedRenderStates)
			, m_shaders(shaders)
		{
			m_mergedRenderStatesKey = mergedRenderStates.key();

			base::CRC64 crc;
			crc << m_shaders->key();
			crc << m_passLayout->key();
			crc << m_mergedRenderStatesKey;
			m_key = crc;
		}

		IBaseGraphicsPipeline::~IBaseGraphicsPipeline()
		{}

		//--

    } // api
} // rendering
