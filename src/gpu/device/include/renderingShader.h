/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

#include "renderingObject.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//----

/// shaders (as uploaded to device)
class GPU_DEVICE_API ShaderObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(ShaderObject, IDeviceObject);

public:
	ShaderObject(ObjectID id, IDeviceObjectHandler* impl, const ShaderMetadata* metadata);

	//--

	// metadata (cached)
	INLINE const ShaderMetadata* metadata() const { return m_metadata; }
		
    //--

	// create the graphics pipeline object with know pass layout and render states
	// NOTE: this may start background shader compilation
	virtual GraphicsPipelineObjectPtr createGraphicsPipeline(const GraphicsRenderStatesObject* renderStats = nullptr) = 0;

	// create the compute pipeline object
	// NOTE: this may start background shader compilation
	virtual ComputePipelineObjectPtr createComputePipeline() = 0;

	//--

private:
	ShaderMetadataPtr m_metadata = nullptr; // from original data
};

//----

END_BOOMER_NAMESPACE_EX(gpu)
