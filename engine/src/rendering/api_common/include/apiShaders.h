/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"
#include "rendering/device/include/renderingShaderLibrary.h"

namespace rendering
{
	namespace api
	{
		//---

		/// type of shader
		enum class ShaderTypeBit : uint16_t
		{
			VertexShaderBit,
			GeometryShaderBit,
			HullShaderBit,
			DomainShaderBit,
			PixelShaderBit,
			ComputeShaderBit,

		};

		//---

		/// loaded shaders, this object mainly servers as caching interface to object cache
		class RENDERING_API_COMMON_API IBaseShaders : public IBaseObject
		{
		public:
			IBaseShaders(IBaseThread* drv, const ShaderLibraryData* data, PipelineIndex index);
			virtual ~IBaseShaders();

			static const auto STATIC_TYPE = ObjectType::Shaders;

			//--

			// what shaders do we have in the bundle
			INLINE ShaderTypeMask mask() const { return m_mask; }

			// key that identifies the shader bundle (can be used to find compiled data in cache)
			INLINE uint64_t key() const { return m_key; }

			// source platform-independent data
			INLINE const ShaderLibraryData* data() const { return m_data; }

			// shader bundle index in the shader library
			INLINE PipelineIndex index() const { return m_index; }

			// vertex layout to use with shaders (NULL for compute shaders)
			INLINE IBaseVertexBindingLayout* vertexLayout() const { return m_vertexLayout; }

			// descriptor layout to use with shaders
			INLINE IBaseDescriptorBindingLayout* descriptorLayout() const { return m_descriptorLayout; }

			//--

			// create graphical rendering pipeline using these shaders that is compatible with given rendering pass and rendering states
			virtual IBaseGraphicsPipeline* createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const IBaseGraphicsRenderStates* renderStates) = 0;

			// create graphical rendering pipeline using these shaders that is compatible with given rendering pass and rendering states
			virtual IBaseComputePipeline* createComputePipeline_ClientApi() = 0;

			//--

		private:
			uint64_t m_key = 0;
			ShaderTypeMask m_mask;

			ShaderLibraryDataPtr m_data;
			PipelineIndex m_index;

			IBaseVertexBindingLayout* m_vertexLayout = nullptr;
			IBaseDescriptorBindingLayout* m_descriptorLayout = nullptr;
		};

		//---

		// client side proxy for shaders object
		class RENDERING_API_COMMON_API ShadersObjectProxy : public ShaderObject
		{
		public:
			ShadersObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const ShaderLibraryData* data, PipelineIndex index);

			virtual GraphicsPipelineObjectPtr createGraphicsPipeline(const GraphicsPassLayoutObject* passLayout, const GraphicsRenderStatesObject* renderStats) override;
			virtual ComputePipelineObjectPtr createComputePipeline() override;
		};

		//---

	} // api
} // rendering