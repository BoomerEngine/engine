/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

#include "descriptorID.h"
#include "samplerState.h"
#include "graphicsStates.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

namespace shader
{
	struct StubProgram;
} // shader

//---

struct GPU_DEVICE_API ShaderVertexElementMetadata
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderVertexElementMetadata);

public:
	StringID name; // "VertexPosition"
	ImageFormat format = ImageFormat::UNKNOWN; // RGB32F
	uint16_t offset = 0; // offset in the stream
	uint16_t size = 0; // field size (can be derived from format, stored for completeness)

	INLINE ShaderVertexElementMetadata() {};

	void print(IFormatStream& f) const;
};

//---

struct GPU_DEVICE_API ShaderVertexStreamMetadata
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderVertexStreamMetadata);

public:
	StringID name; // "SimpleVertex3D"
	uint8_t index = 0; // binding index (internal)
	uint16_t size = 0; // size of the declared vertex data (all elements)
	uint16_t stride = 0; // stride of the vertex element (mostly the same as size)
	bool instanced = false; // is this instance data stream

	Array<ShaderVertexElementMetadata> elements;

	INLINE ShaderVertexStreamMetadata() {};

	void print(IFormatStream& f) const;
};

//---

struct GPU_DEVICE_API ShaderDescriptorEntryMetadata
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderDescriptorEntryMetadata);

public:
	StringID name; // "SimpleVertex3D"
	uint8_t index = 0; // index in descriptor table

	DeviceObjectViewType type = DeviceObjectViewType::Invalid; // type of data
	ShaderStageMask stageMask; // which stages actually use this data, can be used to detect PixelShaderOnly states

	//--

	ImageFormat format = ImageFormat::UNKNOWN; // format of data for formatted descriptors
	ImageViewType viewType = ImageViewType::View2D; // type of texture view
	int number = 0; // stride of data for structured descriptors, size of constant buffers, index of sample for samples 

	INLINE ShaderDescriptorEntryMetadata() {};

	void print(IFormatStream& f) const;
};

//---

struct GPU_DEVICE_API ShaderDescriptorMetadata
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderDescriptorMetadata);

public:
	StringID name; // "TestData"
	DescriptorID id; // set on load, not stored directly
	uint8_t index = 0; // descriptor index in shader metadata

	ShaderStageMask stageMask; // which stages actually use this data, can be used to detect PixelShaderOnly states, OR of all elements

	Array<ShaderDescriptorEntryMetadata> elements;

	//--

	INLINE ShaderDescriptorMetadata() {};

	void print(IFormatStream& f) const;
};

//---

struct GPU_DEVICE_API ShaderStaticSamplerMetadata
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderStaticSamplerMetadata);

public:
	StringID name; // "PointClamp"
	ShaderStageMask stageMask; // which stages actually have shaders :)

	SamplerState state; // actual sampler state

	INLINE ShaderStaticSamplerMetadata() {};

	void print(IFormatStream& f) const;
};

//---

// extracted meta data from compiled shaders (a whole bunch of them for all stages)
// this is ultra-useful information that can be used for client-side validation 
// NOTE: metadata object is always present, 
class GPU_DEVICE_API ShaderMetadata : public IObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(ShaderMetadata, IObject);

public:
	ShaderStageMask stageMask; // which stages actually have shaders :)

	uint64_t key = 0; // unique, content based key for the shader bundle, can be used for caching
	uint64_t vertexLayoutKey = 0; // key describing the vertex data layout
	uint64_t descriptorLayoutKey = 0; // key describing the root signature (all parametrs + static shaders, etc)

	uint16_t computeGroupSizeX = 0;
	uint16_t computeGroupSizeY = 0;
	uint16_t computeGroupSizeZ = 0;

	ShaderFeatureMask featureMask;

	Array<ShaderDescriptorMetadata> descriptors;
	Array<ShaderVertexStreamMetadata> vertexStreams;
	Array<ShaderStaticSamplerMetadata> staticSamplers;

	GraphicsRenderStatesSetup renderStates;

	ShaderMetadata();

	void print(IFormatStream& f) const;

	static ShaderMetadataPtr BuildFromStubs(const shader::StubProgram* program, uint64_t key);

private:
	virtual void onPostLoad() override;

	void cacheRuntimeData();
};

//---

END_BOOMER_NAMESPACE_EX(gpu)
