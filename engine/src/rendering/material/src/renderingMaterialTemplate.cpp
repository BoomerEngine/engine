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
#include "renderingMaterialRuntimeTechnique.h"

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "base/resources/include/resourceFactory.h"
#include "base/object/include/rttiDataView.h"

namespace rendering
{

    ///---

    RTTI_BEGIN_TYPE_ENUM(MaterialSortGroup);
        RTTI_ENUM_OPTION(Opaque);
        RTTI_ENUM_OPTION(OpaqueMasked);
        RTTI_ENUM_OPTION(Transparent);
        RTTI_ENUM_OPTION(Refractive);
    RTTI_END_TYPE();

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

    RTTI_BEGIN_TYPE_CLASS(MaterialPrecompiledStaticTechnique)
        RTTI_PROPERTY(setup);
        RTTI_PROPERTY(renderStates);
        RTTI_PROPERTY(shader);
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterialTemplateDynamicCompiler);
    RTTI_END_TYPE();

    IMaterialTemplateDynamicCompiler::~IMaterialTemplateDynamicCompiler()
    {}

    ///---

    MaterialPrecompiledStaticTechnique::MaterialPrecompiledStaticTechnique()
    {}

    MaterialPrecompiledStaticTechnique::~MaterialPrecompiledStaticTechnique()
    {}

    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialTemplate);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4mt");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Material Template");
        //RTTI_METADATA(base::res::ResourceBakedOnlyMetadata);
        RTTI_PROPERTY(m_parameters);
        RTTI_PROPERTY(m_sortGroup);
        RTTI_PROPERTY(m_compiler);
        RTTI_PROPERTY(m_precompiledTechniques);
    RTTI_END_TYPE();

    MaterialTemplate::MaterialTemplate()
    {
    }

    MaterialTemplate::MaterialTemplate(base::Array<MaterialTemplateParamInfo>&& params, MaterialSortGroup sortGroup, const MaterialTemplateDynamicCompilerPtr& compiler)
        : m_parameters(std::move(params))
        , m_sortGroup(sortGroup)
        , m_compiler(compiler)
    {
        if (m_compiler)
            m_compiler->parent(this);

        rebuildParameterMap();
        registerLayout();
        createDataProxy();

        m_techniqueMap.reserve(32);
    }

    MaterialTemplate::MaterialTemplate(base::Array<MaterialTemplateParamInfo>&& params, MaterialSortGroup sortGroup, base::Array<MaterialPrecompiledStaticTechnique>&& precompiledTechniques)
        : m_parameters(std::move(params))
        , m_sortGroup(sortGroup)
        , m_precompiledTechniques(std::move(precompiledTechniques))
    {
        for (auto& tech : m_precompiledTechniques)
            if (tech.shader)
                tech.shader->parent(this);

        rebuildParameterMap();
        registerLayout();
        createDataProxy();

        m_techniqueMap.reserve(32);
    }

    MaterialTemplate::~MaterialTemplate()
    {
        TRACE_INFO("Releasing material template '{}'", key());
        m_techniqueMap.clear();
        m_compiler.reset();
        TRACE_INFO("Released material template '{}'", key());
    }

    const MaterialTemplateParamInfo* MaterialTemplate::findParameterInfo(base::StringID name) const
    {
        uint16_t index = 0;
        if (m_parametersMap.find(name, index))
            return &m_parameters[index];

        return nullptr;
    }

    void MaterialTemplate::onPostLoad()
    {
        TBaseClass::onPostLoad();
        rebuildParameterMap();
        registerLayout();
        createDataProxy();
    }

    void MaterialTemplate::registerLayout()
    {
        base::Array<MaterialDataLayoutEntry> layoutEntries;
        layoutEntries.reserve(m_parameters.size());

        for (const auto& param : m_parameters)
        { 
            auto& entry = layoutEntries.emplaceBack();
            entry.name = param.name;
            entry.type = param.parameterType;
        }

        // TODO: optimize layout/order

        std::sort(layoutEntries.begin(), layoutEntries.end(), [](const MaterialDataLayoutEntry& a, MaterialDataLayoutEntry& b) { return a.name.view() < b.name.view(); });

        m_dataLayout = base::GetService<MaterialService>()->registerDataLayout(std::move(layoutEntries));
    }

    void MaterialTemplate::rebuildParameterMap()
    {
        m_parametersMap.reserve(m_parameters.size());

        for (uint32_t i = 0; i < m_parameters.size(); ++i)
            m_parametersMap[m_parameters[i].name] = i;
    }

    ///---

    MaterialTechniquePtr MaterialTemplate::fetchTechnique(const MaterialCompilationSetup& setup)
    {
        const auto key = setup.key();

        // look in dynamic list
        auto lock = base::CreateLock(m_techniqueMapLock);
        auto& ret = m_techniqueMap[key];
        if (!ret)
        {
            ret = MemNew(MaterialTechnique, setup);

            // lookup in precompiled list
            for (const auto& techniqe : m_precompiledTechniques)
            {
                if (key == techniqe.setup.key())
                {
                    auto compiledTechnique = MemNew(MaterialCompiledTechnique).ptr;
                    compiledTechnique->shader = techniqe.shader;
                    compiledTechnique->dataLayout = m_dataLayout;
                    compiledTechnique->renderStates = techniqe.renderStates;
                    ret->pushData(compiledTechnique);
                    break;
                }
            }

            // try to compile dynamically
            if (m_compiler)
                m_compiler->requestTechniqueComplation(path().view(), ret);
        }

        return ret;
    }

    RTTI_BEGIN_TYPE_CLASS(MaterialCompilationSetup);
        RTTI_PROPERTY(pass);
        RTTI_PROPERTY(vertexFormat);
        RTTI_PROPERTY(vertexFetchMode);
        RTTI_PROPERTY(msaa);
    RTTI_END_TYPE();

    uint32_t MaterialCompilationSetup::key() const
    {
        uint32_t ret = 0;
        uint8_t bitCount = 0;
#define MERGE_BITS(data, num) { ret |= (uint32_t)(data) << bitCount; bitCount += num; }
        MERGE_BITS(msaa ? 1 : 0, 1);
        MERGE_BITS(vertexFormat, 3);
        MERGE_BITS(vertexFetchMode, 1);
        MERGE_BITS(pass, 3);
#undef MERGE_BITS
        return ret;
    }

    void MaterialCompilationSetup::print(base::IFormatStream& f) const
    {
        f.appendf("PASS={}", pass);
        f.appendf(" VERTEX={}", vertexFormat);

        if (vertexFetchMode)
            f.appendf(" FETCH={}", vertexFetchMode);

        if (msaa)
            f.append(" MSAA");
    }

    ///---

    MaterialDataProxyPtr MaterialTemplate::dataProxy() const
    {
        return m_dataProxy;
    }

    const MaterialTemplate* MaterialTemplate::resolveTemplate() const
    {
        return this;
    }

    bool MaterialTemplate::resetParameterRaw(base::StringID name)
    {
        return false;
    }

    bool MaterialTemplate::writeParameterRaw(base::StringID name, const void* data, base::Type type, bool refresh /*= true*/)
    {
        return false; // templates are read only
    }

    bool MaterialTemplate::readParameterRaw(base::StringID name, void* data, base::Type type, bool defaultValueOnly /*= false*/) const
    {
        if (const auto* info = findParameterInfo(name))
            return info->defaultValue.get(data, type);

        return false;
    }

    ///---

    bool MaterialTemplate::readDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        if (TBaseClass::readDataView(rootView, rootViewPath, viewPath, targetData, targetType))
            return true;

        if (!viewPath.empty())
        {
            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName) && !propertyName.empty())
            {
                // we don't have the notion of "reset to default value" for material template
                if (targetType == base::rtti::DataViewBaseValue::GetStaticClass())
                    return false;

                if (const auto* paramBlock = findParameterInfo(propertyName))
                {
                    return paramBlock->type->readDataView(nullptr, nullptr, "", viewPath, paramBlock->defaultValue.data(), targetData, targetType);
                }
            }
        }

        return false;
    }

    bool MaterialTemplate::writeDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, const void* sourceData, base::Type sourceType)
    {
        if (TBaseClass::writeDataView(rootView, rootViewPath, viewPath, sourceData, sourceType))
            return true;

        return false;
    }

    bool MaterialTemplate::describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        base::StringView<char> propertyName;
        if (viewPath.empty())
        {
            if (!TBaseClass::describeDataView(viewPath, outInfo))
                return false;

            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
            {
                for (const auto& parameterBlock : m_parameters)
                {
                    auto& memberInfo = outInfo.members.emplaceBack();
                    memberInfo.name = parameterBlock.name;
                    memberInfo.category = parameterBlock.category;
                }
            }

            return true;
        }
        else
        {
            if (TBaseClass::describeDataView(viewPath, outInfo))
                return true;

            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (auto* paramBlock = findParameterInfo(propertyName))
                    return paramBlock->type->describeDataView(viewPath, paramBlock->defaultValue.data(), outInfo);
            }
        }

        return false;
    }

    ///---

    void MaterialTemplate::createDataProxy()
    {
        DEBUG_CHECK_EX(!m_dataProxy, "Data proxy for material template should be immutable");
        m_dataProxy = MemNew(MaterialDataProxy, this, false, *this); // NOTE: we DO NOT keep extra ref here
    }

    ///---

} // rendering
