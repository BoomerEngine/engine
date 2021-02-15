/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

namespace rendering
{

    //----

    // Shader object - not a resource since it's usually embedded in one and it can be different ones
	// for a shader based resource look for "ShaderFile" resource
	// Shader consists of two parts: 
	//  - payload - saved as portable shader opcodes (not specific to any API)
	//  - metadata - mostly for client side use and validation
    class RENDERING_DEVICE_API ShaderData : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ShaderData, base::IObject);

    public:
		ShaderData();
		ShaderData(base::Buffer data, const ShaderMetadata* metadata); // metadata is parented to the shader
		virtual ~ShaderData();

        //--

		// get compressed shader data (only unpacked if compilation is required)
		INLINE const base::Buffer& data() const { return m_data; }

		// get shader matadata, contains higher level description of shader
		INLINE const ShaderMetadata* metadata() const { return m_metadata; }

		//--

		// get the rendering update uploaded shaders
		// NOTE: still have to create pipelines
		INLINE const ShaderObjectPtr& deviceShader() const { return m_deviceShader; }

		//--

	private:
		ShaderMetadataPtr m_metadata;
		base::Buffer m_data;

		//--

		ShaderObjectPtr m_deviceShader;

		//--

		virtual void onPostLoad() override;

		void createDeviceObjects();
		void destroyDeviceObjects();
    };

    //----

} // rendering
