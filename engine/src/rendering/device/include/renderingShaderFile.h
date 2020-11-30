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

	/// single shader loaded from a file on disk, most common case for technical shaders, post processes
	class RENDERING_DEVICE_API ShaderFile : public base::res::IResource
	{
		RTTI_DECLARE_VIRTUAL_CLASS(ShaderFile, base::res::IResource);

	public:
		ShaderFile();
		ShaderFile(const ShaderDataPtr& data);
		virtual ~ShaderFile();

		//--

		// get main shader
		// TODO: implement 
		INLINE const ShaderDataPtr& shader() const { return m_shader; }

		//--

	private:
		ShaderDataPtr m_shader;
	};

	//----

} // rendering
