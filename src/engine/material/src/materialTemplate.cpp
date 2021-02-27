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
#include "core/resource/include/resourceFactory.h"
#include "core/object/include/rttiDataView.h"
#include "core/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParamInfo);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(category);
    RTTI_PROPERTY(type);
    RTTI_PROPERTY(defaultValue);
    RTTI_PROPERTY(automaticTextureSuffix);
    RTTI_PROPERTY(parameterType);
RTTI_END_TYPE();

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

bool MaterialTemplate::checkParameterOverride(StringID name) const
{
    return false;
}

bool MaterialTemplate::resetParameter(StringID name)
{
    return false;
}

bool MaterialTemplate::writeParameter(StringID name, const void* data, Type type, bool refresh /*= true*/)
{
    return false; // templates are read only
}

bool MaterialTemplate::readParameter(StringID name, void* data, Type type) const
{
    MaterialTemplateParamInfo info;
    if (queryParameterInfo(name, info))
        return info.defaultValue.get(data, type);

    return false;
}

bool MaterialTemplate::readBaseParameter(StringID name, void* data, Type type) const
{
    return readParameter(name, data, type);
}

/*const void* MaterialTemplate::findParameterDataInternal(StringID name, Type& outType) const
{
    MaterialTemplateParamInfo info;
    if (queryParameterInfo(name, info))
    {
        outType = info.defaultValue.type();
        return info.defaultValue.data();
    }

    return nullptr;
}*/

const void* MaterialTemplate::findBaseParameterDataInternal(StringID name, Type& outType) const
{
    return findParameterDataInternal(name, outType);
}

///---

DataViewResult MaterialTemplate::readDataView(StringView viewPath, void* targetData, Type targetType) const
{
    const auto originalPath = viewPath;

    StringView propertyName;
    if (rtti::ParsePropertyName(viewPath, propertyName))
    {
        MaterialTemplateParamInfo info;
        if (queryParameterInfo(propertyName, info))
        {
            return info.type->readDataView(viewPath, info.defaultValue.data(), targetData, targetType);
        }
    }

    return TBaseClass::readDataView(originalPath, targetData, targetType);
}

DataViewResult MaterialTemplate::describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const
{
    const auto orgViewPath = viewPath;

    if (viewPath.empty())
    {
        if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::MemberList))
            listParameters(outInfo);
    }
    else
    {
        StringView propertyName;
        if (rtti::ParsePropertyName(viewPath, propertyName))
        {
            MaterialTemplateParamInfo info;
            if (queryParameterInfo(propertyName, info))
            {
                return info.type->describeDataView(viewPath, info.defaultValue.data(), outInfo);
            }
        }
    }

    return TBaseClass::describeDataView(orgViewPath, outInfo);
}

DataViewResult MaterialTemplate::describeParameterView(StringView paramName, StringView viewPath, rtti::DataViewInfo& outInfo) const
{
    MaterialTemplateParamInfo info;
    if (queryParameterInfo(paramName, info))
    {
        return info.type->describeDataView(viewPath, info.defaultValue.data(), outInfo);
    }

    return DataViewResultCode::ErrorUnknownProperty;
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

    Array<MaterialTemplateParamInfo> parameters;
    queryAllParameterInfos(parameters);

    MaterialTemplateMetadata metadata;
    queryMatadata(metadata);

    Array<MaterialPrecompiledStaticTechnique> precompiledTechniques;
       

    const auto& contextPath = path() ? path().str() : m_contextPath;
	m_templateProxy = RefNew<MaterialTemplateProxy>(contextPath, parameters, metadata, queryDynamicCompiler(), precompiledTechniques);
}

///---

END_BOOMER_NAMESPACE()
