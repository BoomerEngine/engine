/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialInstance.h"
#include "renderingMaterialTemplate.h"
#include "renderingMaterialRuntimeProxy.h"
#include "renderingMaterialRuntimeService.h"

#include "rendering/texture/include/renderingTexture.h"

#include "base/resources/src/resourceGeneralTextLoader.h"
#include "base/resources/src/resourceGeneralTextSaver.h"
#include "base/resources/include/resourceSerializationMetadata.h"
#include "base/resources/include/resourceFactory.h"
#include "base/object/include/rttiDataView.h"

namespace base
{
    namespace rtti
    {
        extern BASE_OBJECT_API bool PatchResourceReferences(Type type, void* data, res::IResource* currentResource, res::IResource* newResource);
    }
}

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialInstanceParam);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(value);
        RTTI_PROPERTY(loadedTexture);
    RTTI_END_TYPE();

    //----

    // factory class for the material instance
    class MaterialInstanceFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<MaterialInstance>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialInstanceFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<MaterialInstance>();
    RTTI_END_TYPE();


    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialInstance);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4mi");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Material Instance");
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0x4c, 0x5b, 0x61);
        //RTTI_METADATA(base::SerializationLoaderMetadata).bind<base::res::text::TextLoader>();
        //RTTI_METADATA(base::SerializationSaverMetadata).bind<base::res::text::TextSaver>();
        RTTI_PROPERTY(m_parameters);
        RTTI_PROPERTY(m_baseMaterial);
    RTTI_END_TYPE();

    MaterialInstance::MaterialInstance()
    {
        createMaterialProxy();
    }

    MaterialInstance::MaterialInstance(const MaterialRef& baseMaterialRef)
    {
        baseMaterial(baseMaterialRef);
        createMaterialProxy();
    }

    MaterialInstance::~MaterialInstance()
    {
    }

    static const base::StringView<char> BASE_MATERIAL = "BaseMaterial";

    static base::res::StaticResource<MaterialTemplate> resFallbackMaterial("engine/materials/fallback.v4mg");

    MaterialDataProxyPtr MaterialInstance::dataProxy() const
    {
        // use existing proxy
        if (m_dataProxy)
            return m_dataProxy;

        // use proxy from base material
        if (auto base = m_baseMaterial.acquire())
            return base->dataProxy();

        // use fallback proxy
        auto fallbackTemplate = resFallbackMaterial.loadAndGet();
        DEBUG_CHECK_EX(fallbackTemplate, "Failed to load fallback material template, rendering pipeline is seriously broken");
        return fallbackTemplate->dataProxy();
    }

    const MaterialTemplate* MaterialInstance::resolveTemplate() const
    {
        if (auto base = m_baseMaterial.acquire())
            if (auto baseTemplate = base->resolveTemplate())
                return baseTemplate;

        auto fallbackTemplate = resFallbackMaterial.loadAndGet();
        DEBUG_CHECK_EX(fallbackTemplate, "Failed to load fallback material template, rendering pipeline is seriously broken");
        return fallbackTemplate;
    }

    bool MaterialInstance::resetParameterRaw(base::StringID name)
    {
        const auto* params = m_parameters.typedData();
        for (uint32_t i = 0; i < m_parameters.size(); ++i)
        {
            if (params[i].name == name)
            {
                markModified();
                m_parameters.eraseUnordered(i);
                onPropertyChanged(name.view());
                return true;
            }
        }

        return false;
    }

    bool MaterialInstance::writeParameterRaw(base::StringID name, const void* data, base::Type type, bool refresh /*= true*/)
    {
        for (auto& param : m_parameters)
        {
            if (param.name == name)
            {
                if (param.value.set(data, type))
                {
                    markModified();
                    return true;
                }
            }
        }

        if (auto materialTemplate = resolveTemplate())
        {
            if (const auto* paramInfo = materialTemplate->findParameterInfo(name))
            {
                auto value = paramInfo->defaultValue;
                if (base::rtti::ConvertData(data, type, value.data(), value.type()))
                {
                    auto& entry = m_parameters.emplaceBack();
                    entry.name = base::StringID(name);
                    entry.value = std::move(value);

                    onPropertyChanged(name.view());
                    return true;
                }
            }
        }

        return false;
    }

    bool MaterialInstance::readParameterRaw(base::StringID name, void* data, base::Type type, bool defaultValueOnly /*= false*/) const
    {
        if (!defaultValueOnly)
            for (const auto& param : m_parameters)
                if (param.name == name)
                    return param.value.get(data, type);

        if (auto base = m_baseMaterial.acquire())
            return base->readParameterRaw(name, data, type);
        
        return false;
    }

    ///---

    void MaterialInstance::removeAllParameters()
    {
        if (!m_parameters.empty())
        {
            auto oldParams = std::move(m_parameters);
            notifyDataChanged();

            for (const auto& param : oldParams)
                TBaseClass::onPropertyChanged(param.name.view());
        }
    }

    void MaterialInstance::baseMaterial(const MaterialRef& baseMaterial)
    {
        if (m_baseMaterial != baseMaterial)
        {
            m_baseMaterial = baseMaterial;
            onPropertyChanged(BASE_MATERIAL);
        }
    }

    void MaterialInstance::onPostLoad()
    {
        TBaseClass::onPostLoad();

        for (int i = m_parameters.lastValidIndex(); i >= 0; --i)
            if (m_parameters[i].value.type() == nullptr)
                m_parameters.erase(i);

        for (auto& param : m_parameters)
        {
            if (param.loadedTexture)
            {
                TRACE_INFO("Preloaded texture for '{}': '{}'", param.name, param.loadedTexture.acquire()->path().view());
            }
        }

        changeTrackedMaterial(m_baseMaterial.acquire());
        createMaterialProxy();
    }

    void MaterialInstance::notifyDataChanged()
    {
        createMaterialProxy();
        TBaseClass::notifyDataChanged();
    }

    void MaterialInstance::notifyBaseMaterialChanged()
    {
        postEvent("OnFullStructureChange"_id);
        createMaterialProxy();
        TBaseClass::notifyBaseMaterialChanged();
    }

    void MaterialInstance::onPropertyChanged(base::StringView<char> path)
    {
        auto orgPropName = path;

        if (path == BASE_MATERIAL)
        {
            const auto baseMaterial = m_baseMaterial.acquire();
            changeTrackedMaterial(baseMaterial);
            notifyBaseMaterialChanged();
        }
        else
        {
            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(path, propertyName) && !propertyName.empty())
            {
                if (const auto* baseTemplate = resolveTemplate())
                {
                    if (baseTemplate->findParameterInfo(base::StringID(propertyName)))
                    {
                        notifyDataChanged();
                    }
                }
            }
        }

        TBaseClass::onPropertyChanged(orgPropName);
    }

    void MaterialInstance::createMaterialProxy()
    {
        // resolve the base template that will be used to create the material proxy
        if (const auto* baseTemplate = resolveTemplate())
        {
            // check if can just update it
            if (m_dataProxy && m_dataProxy->materialTemplate() == baseTemplate && m_dataProxy->layout() == baseTemplate->dataLayout())
            {
                m_dataProxy->update(*this);
            }
            else
            {
                auto oldProxy = m_dataProxy;
                m_dataProxy = MemNew(MaterialDataProxy, baseTemplate, true, *this); // we keep extra ref on the template

                base::GetService<MaterialService>()->notifyMaterialProxyChanged(oldProxy, m_dataProxy);
            }
        }
    }

    //--

    bool MaterialInstance::onResourceReloading(base::res::IResource* currentResource, base::res::IResource* newResource)
    {
        bool ret = TBaseClass::onResourceReloading(currentResource, newResource);

        for (auto& param : m_parameters)
        {
            if (base::rtti::PatchResourceReferences(param.value.type(), param.value.data(), currentResource, newResource))
            {
                ret = true;
            }
        }

        return ret;
    }

    void MaterialInstance::onResourceReloadFinished(base::res::IResource* currentResource, base::res::IResource* newResource)
    {
        TBaseClass::onResourceReloadFinished(currentResource, newResource);
        createMaterialProxy();
    }

    bool MaterialInstance::resetBase()
    {
        return false; // not resetable
    }

    bool MaterialInstance::hasParameterOverride(const base::StringID name) const
    {
        for (const auto& paramInfo : m_parameters)
            if (paramInfo.name == name && !paramInfo.value.empty())
                return true;

        return false;
    }

    bool MaterialInstance::readDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        if (viewPath.empty())
            return false;

        base::StringView<char> originalViewPath = viewPath;
        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            // general comparison vs base property
            if (targetType == base::rtti::DataViewBaseValue::GetStaticClass())
            {
                if (!viewPath.empty())
                    return false; // we do not track sub-values

                auto& ret = *(base::rtti::DataViewBaseValue*)targetData;
                ret.differentThanBase = hasParameterOverride(propertyName);
                return true;
            }

            // base material pointer
            if (propertyName == BASE_MATERIAL)
                return base::reflection::GetTypeObject<MaterialRef>()->readDataView((base::IObject*)this, rootView, rootViewPath, viewPath, &m_baseMaterial, targetData, targetType);

            // read value
            for (const auto& paramInfo : m_parameters)
                if (paramInfo.name == propertyName && !paramInfo.value.empty())
                    return paramInfo.value.type()->readDataView(nullptr, nullptr, "", viewPath, paramInfo.value.data(), targetData, targetType);

            // read default value from base material
            if (const auto baseMaterial = m_baseMaterial.acquire())
                return baseMaterial->readDataView(rootView, rootViewPath, originalViewPath, targetData, targetType);
        }

        return false;
    }

    bool MaterialInstance::writeDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, const void* sourceData, base::Type sourceType)
    {
        if (viewPath.empty())
            return false;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName) && !propertyName.empty())
        {
            // reset command
            if (sourceType == base::rtti::DataViewCommand::GetStaticClass())
            {
                const auto& cmd = *(const base::rtti::DataViewCommand*)sourceData;

                if (cmd.command == "reset"_id)
                {
                    if (propertyName == BASE_MATERIAL)
                        return resetBase();
                    else
                        return resetParameterRaw(base::StringID(propertyName));
                }

                return false;
            }

            // base material
            if (propertyName == BASE_MATERIAL)
            {
                if (!base::reflection::GetTypeObject<MaterialRef>()->writeDataView((base::IObject*)this, rootView, rootViewPath, viewPath, &m_baseMaterial, sourceData, sourceType))
                    return false;
                onPropertyChanged(BASE_MATERIAL);
                return true;
            }

            // find base type info
            if (const auto* materialTemplate = resolveTemplate())
            {
                if (const auto* paramInfo = materialTemplate->findParameterInfo(base::StringID(propertyName)))
                {
                    // ALWAYS write to compatible type
                    auto value = paramInfo->defaultValue;
                    if (value.type()->writeDataView(nullptr, nullptr, "", viewPath, value.data(), sourceData, sourceType))
                    {
                        // update if we already have it
                        bool createNew = true;
                        for (auto& paramInfo : m_parameters)
                        {
                            if (paramInfo.name == propertyName)
                            {
                                paramInfo.value = value;
                                createNew = false;
                                break;
                            }
                        }

                        // or write a new one
                        if (createNew)
                        {
                            auto& entry = m_parameters.emplaceBack();
                            entry.name = base::StringID(propertyName);
                            entry.value = value;
                        }

                        onPropertyChanged(propertyName);
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool MaterialInstance::describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        if (viewPath.empty())
        {
            if (!TBaseClass::describeDataView(viewPath, outInfo))
                return false;

            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
            {
                auto& propInfo = outInfo.members.emplaceBack();
                propInfo.name = BASE_MATERIAL;
                propInfo.category = "Material Hierarchy"_id;

                if (const auto* materialTemplate = resolveTemplate())
                    return materialTemplate->describeDataView(viewPath, outInfo);
            }

            return true;
        }
        else
        {
            if (const auto* materialTemplate = resolveTemplate())
                return materialTemplate->describeDataView(viewPath, outInfo);

            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName) && !propertyName.empty())
            {
                if (propertyName == BASE_MATERIAL)
                    return base::reflection::GetTypeObject<MaterialRef>()->describeDataView(viewPath, &m_baseMaterial, outInfo);
            }
        }

        return false;
    }

    //---

} // rendering