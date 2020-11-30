/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"

#include "renderingShader.h"
#include "renderingShaderData.h"
#include "renderingShaderMetadata.h"
#include "renderingDeviceApi.h"
#include "renderingDeviceService.h"

namespace rendering
{

    //--

	RTTI_BEGIN_TYPE_CLASS(ShaderData);
		RTTI_PROPERTY(m_data);
		RTTI_PROPERTY(m_metadata);
	RTTI_END_TYPE();

	ShaderData::ShaderData()
	{}

	ShaderData::ShaderData(base::Buffer data, const ShaderMetadata* metadata)
		: m_data(data)
		, m_metadata(AddRef(metadata))
	{
		m_metadata->parent(this);
		createDeviceObjects();
	}

	ShaderData::~ShaderData()
	{
		destroyDeviceObjects();
	}

	void ShaderData::onPostLoad()
	{
		TBaseClass::onPostLoad();
	}

	void ShaderData::createDeviceObjects()
	{
		DEBUG_CHECK_RETURN_EX(!m_data.empty(), "No source shader data");
		DEBUG_CHECK_RETURN_EX(m_metadata, "No source metadata");

		if (auto* service = base::GetService<DeviceService>())
			if (auto* device = service->device())
				m_deviceShader = device->createShaders(this);
	}

	void ShaderData::destroyDeviceObjects()
	{
		m_deviceShader.reset();
	}

    //--

} // rendering
