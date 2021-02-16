/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialTemplate.h"
#include "renderingMaterialInstance.h"
#include "renderingMaterialRuntimeService.h"
#include "renderingMaterialRuntimeLayout.h"
#include "renderingMaterialRuntimeProxy.h"
#include "renderingMaterialRuntimeTemplate.h"
#include "renderingMaterialRuntimeTechnique.h"

#include "rendering/device/include/renderingShaderData.h"
#include "base/resource/include/resourceFactory.h"
#include "base/object/include/rttiDataView.h"
#include "base/resource/include/resourceTags.h"

namespace rendering
{

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

	base::StringID MeshVertexFormatBindPointName(MeshVertexFormat format)
	{
		return base::StringID(base::TempString("Vertex{}", format));
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

    void MaterialCompilationSetup::print(base::IFormatStream& f) const
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

    bool MaterialTemplate::checkParameterOverride(base::StringID name) const
    {
        return false;
    }

    bool MaterialTemplate::resetParameter(base::StringID name)
    {
        return false;
    }

    bool MaterialTemplate::writeParameter(base::StringID name, const void* data, base::Type type, bool refresh /*= true*/)
    {
        return false; // templates are read only
    }

    bool MaterialTemplate::readParameter(base::StringID name, void* data, base::Type type) const
    {
        MaterialTemplateParamInfo info;
        if (queryParameterInfo(name, info))
            return info.defaultValue.get(data, type);

        return false;
    }

    bool MaterialTemplate::readBaseParameter(base::StringID name, void* data, base::Type type) const
    {
        return readParameter(name, data, type);
    }

    /*const void* MaterialTemplate::findParameterDataInternal(base::StringID name, base::Type& outType) const
    {
        MaterialTemplateParamInfo info;
        if (queryParameterInfo(name, info))
        {
            outType = info.defaultValue.type();
            return info.defaultValue.data();
        }

        return nullptr;
    }*/

    const void* MaterialTemplate::findBaseParameterDataInternal(base::StringID name, base::Type& outType) const
    {
        return findParameterDataInternal(name, outType);
    }

    ///---

    base::DataViewResult MaterialTemplate::readDataView(base::StringView viewPath, void* targetData, base::Type targetType) const
    {
        const auto originalPath = viewPath;

        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            MaterialTemplateParamInfo info;
            if (queryParameterInfo(propertyName, info))
            {
                return info.type->readDataView(viewPath, info.defaultValue.data(), targetData, targetType);
            }
        }

        return TBaseClass::readDataView(originalPath, targetData, targetType);
    }

    base::DataViewResult MaterialTemplate::describeDataView(base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        const auto orgViewPath = viewPath;

        if (viewPath.empty())
        {
            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
                listParameters(outInfo);
        }
        else
        {
            base::StringView propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
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

    base::DataViewResult MaterialTemplate::describeParameterView(base::StringView paramName, base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        MaterialTemplateParamInfo info;
        if (queryParameterInfo(paramName, info))
        {
            return info.type->describeDataView(viewPath, info.defaultValue.data(), outInfo);
        }

        return base::DataViewResultCode::ErrorUnknownProperty;
    }

    ///---

    void MaterialTemplate::createDataProxy()
    {
        DEBUG_CHECK_EX(!m_dataProxy, "Data proxy for material template should be immutable");
		DEBUG_CHECK_EX(m_templateProxy, "Missing template proxy");
        m_dataProxy = base::RefNew<MaterialDataProxy>(m_templateProxy);
        m_dataProxy->update(*this);
    }

	void MaterialTemplate::createTemplateProxy()
	{
		DEBUG_CHECK_EX(!m_templateProxy, "Template proxy for material template should be immutable");

        base::Array<MaterialTemplateParamInfo> parameters;
        queryAllParameterInfos(parameters);

        MaterialTemplateMetadata metadata;
        queryMatadata(metadata);

        base::Array<MaterialPrecompiledStaticTechnique> precompiledTechniques;
       

        const auto& contextPath = path() ? path().str() : m_contextPath;
		m_templateProxy = base::RefNew<MaterialTemplateProxy>(contextPath, parameters, metadata, queryDynamicCompiler(), precompiledTechniques);
	}

    ///---

} // rendering
