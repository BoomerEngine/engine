/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "materialTemplate.h"
#include "materialInstance.h"
#include "runtimeService.h"
#include "runtimeLayout.h"
#include "runtimeProxy.h"
#include "runtimeTemplate.h"
#include "runtimeTechnique.h"

#include "gpu/device/include/shaderData.h"
#include "core/resource/include/factory.h"
#include "core/object/include/rttiDataView.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParamInfo);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(defaultValue);
    RTTI_PROPERTY(parameterType);
RTTI_END_TYPE();

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterialTemplateParam);
    RTTI_CATEGORY("Naming");
    RTTI_PROPERTY(m_name); // not editable directly
    RTTI_PROPERTY(m_category).editable("Parameter display category");
    RTTI_CATEGORY("Editor");
    RTTI_PROPERTY(m_hint).editable("User tooltip");
RTTI_END_TYPE();

IMaterialTemplateParam::~IMaterialTemplateParam()
{}

void IMaterialTemplateParam::rename(StringID name)
{
    if (m_name != name)
    {
        m_name = name;
        onPropertyChanged("name");
    }
}

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterialTemplateDynamicCompiler);
RTTI_END_TYPE();

IMaterialTemplateDynamicCompiler::~IMaterialTemplateDynamicCompiler()
{}

///---

RTTI_BEGIN_TYPE_CLASS(MaterialPrecompiledStaticTechnique)
	RTTI_PROPERTY(setup);
	RTTI_PROPERTY(shader);
RTTI_END_TYPE();

MaterialPrecompiledStaticTechnique::MaterialPrecompiledStaticTechnique()
{}

MaterialPrecompiledStaticTechnique::~MaterialPrecompiledStaticTechnique()
{}

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialTemplate);
    RTTI_PROPERTY(m_contextPath);
    RTTI_PROPERTY(m_parameters);
RTTI_END_TYPE();

MaterialTemplate::MaterialTemplate()
{
}

MaterialTemplate::~MaterialTemplate()
{
}

void MaterialTemplate::onPostLoad()
{
    TBaseClass::onPostLoad();

    for (auto& param : m_parameters)
        if (param && !param->name())
            param = nullptr;

    m_parameters.removeAll(nullptr);

    createTemplateProxy();
    createDataProxy();
}

///---

RTTI_BEGIN_TYPE_ENUM(MeshVertexFormat);
	RTTI_ENUM_OPTION(PositionOnly);
	RTTI_ENUM_OPTION(Static);
	RTTI_ENUM_OPTION(StaticEx);
	RTTI_ENUM_OPTION(Skinned4);
	RTTI_ENUM_OPTION(Skinned4Ex);
RTTI_END_TYPE();

StringID MeshVertexFormatBindPointName(MeshVertexFormat format)
{
	return StringID(TempString("Vertex{}", format));
}

///---

RTTI_BEGIN_TYPE_CLASS(MaterialCompilationSetup);
    RTTI_PROPERTY(pass);
    RTTI_PROPERTY(vertexFormat);
    RTTI_PROPERTY(bindlessTextures);
	RTTI_PROPERTY(meshletsVertices);
    RTTI_PROPERTY(msaa);
RTTI_END_TYPE();

uint32_t MaterialCompilationSetup::key() const
{
    uint32_t ret = 0;
    uint8_t bitCount = 0;
#define MERGE_BITS(data, num) { ret |= (uint32_t)(data) << bitCount; bitCount += num; }
    MERGE_BITS(staticSwitches, 10);
    MERGE_BITS(msaa ? 1 : 0, 1);
	MERGE_BITS(bindlessTextures, 1);
	MERGE_BITS(meshletsVertices, 1);
	MERGE_BITS(vertexFormat, 3);
    MERGE_BITS(pass, 3);
#undef MERGE_BITS
    return ret;
}

void MaterialCompilationSetup::print(IFormatStream& f) const
{
    f.appendf("PASS={}", pass);
    f.appendf(" VERTEX={}", vertexFormat);

	if (meshletsVertices)
		f.append(" MESHLET");
	if (bindlessTextures)
		f.append(" BINDLESS");
    if (msaa)
        f.append(" MSAA");
}

///---

MaterialDataProxyPtr MaterialTemplate::dataProxy() const
{
    return m_dataProxy;
}

MaterialTemplateProxyPtr MaterialTemplate::templateProxy() const
{
	return m_templateProxy;
}

const MaterialTemplate* MaterialTemplate::resolveTemplate() const
{
	return this;
}

bool MaterialTemplate::readParameter(StringID name, void* data, Type type) const
{
    for (const auto& param : m_parameters)
        if (param->name() == name)
            return ConvertData(param->queryDefaultValue(), param->queryDataType(), data, type);
    
    return false;
}

///---

const IMaterialTemplateParam* MaterialTemplate::findParameter(StringID name) const
{
    for (const auto& param : m_parameters)
        if (param->name() == name)
            return param;

    return nullptr;
}

///---

void MaterialTemplate::createDataProxy()
{
    DEBUG_CHECK_EX(!m_dataProxy, "Data proxy for material template should be immutable");
	DEBUG_CHECK_EX(m_templateProxy, "Missing template proxy");
    m_dataProxy = RefNew<MaterialDataProxy>(m_templateProxy);
    m_dataProxy->update(*this);
}

void MaterialTemplate::createTemplateProxy()
{
	DEBUG_CHECK_EX(!m_templateProxy, "Template proxy for material template should be immutable");

    Array<MaterialTemplateParamInfo> proxyParameters;
    proxyParameters.reserve(m_parameters.size());

    for (const auto& param : parameters())
    {
        if (param->name())
        {
            if (const auto type = param->queryDataType())
            {
                auto& proxyParam = proxyParameters.emplaceBack();
                proxyParam.name = param->name();
                proxyParam.parameterType = param->queryType();
                proxyParam.defaultValue = Variant(param->queryDataType(), param->queryDefaultValue());
            }
        }
    }

    Array<MaterialPrecompiledStaticTechnique> precompiledTechniques;

    const auto& contextPath = loadPath() ? loadPath() : m_contextPath;
	m_templateProxy = RefNew<MaterialTemplateProxy>(contextPath, proxyParameters, queryDynamicCompiler(), precompiledTechniques);
}

///---

END_BOOMER_NAMESPACE()
