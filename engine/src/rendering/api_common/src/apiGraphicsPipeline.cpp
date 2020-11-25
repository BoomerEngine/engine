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

		IBaseGraphicsPipeline::IBaseGraphicsPipeline(IBaseThread* owner, const IBaseShaders* shaders, const IBaseGraphicsPassLayout* passLayout, const IBaseGraphicsRenderStates* states)
			: IBaseObject(owner, ObjectType::GraphicsPipelineObject)
			, m_passLayout(passLayout)
			, m_renderStates(states)
			, m_shaders(shaders)
		{
			base::CRC64 crc;
			crc << m_passLayout->key();
			crc << m_renderStates->key();
			crc << m_shaders->key();
			m_key = crc;
		}

		IBaseGraphicsPipeline::~IBaseGraphicsPipeline()
		{}

		//--

    } // api
} // rendering
