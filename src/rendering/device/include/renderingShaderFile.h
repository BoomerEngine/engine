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

		// find shader permutation matching given definitions
		const ShaderData* findShader(const ShaderSelector& selector) const;

		// get root shader (no defines)
		const ShaderData* rootShader() const;

		//--

	private:
		ShaderDataPtr m_shader;
	};

	//----

} // rendering
