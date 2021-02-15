/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "renderingShaderData.h"
#include "renderingShaderFile.h"
#include "base/resource/include/resourceTags.h"

namespace rendering
{

    //--

    RTTI_BEGIN_TYPE_CLASS(ShaderFile);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4shader");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Compiled Shader");
        RTTI_PROPERTY(m_shader);
    RTTI_END_TYPE();

	ShaderFile::ShaderFile()
	{}

	ShaderFile::ShaderFile(const ShaderDataPtr& data)
		: m_shader(data)
	{
		m_shader->parent(this);
	}

	ShaderFile::~ShaderFile()
	{}

	//--

	const ShaderData* ShaderFile::rootShader() const
	{
		return m_shader;
	}

	const ShaderData* ShaderFile::findShader(const ShaderSelector& selector) const
	{
		return m_shader;
	}

    //--

} // rendering
