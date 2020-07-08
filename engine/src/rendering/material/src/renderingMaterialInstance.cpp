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

#include "base/resource/src/resourceGeneralTextLoader.h"
#include "base/resource/src/resourceGeneralTextSaver.h"
#include "base/resource/include/resourceSerializationMetadata.h"
#include "base/resource/include/resourceFactory.h"
#include "base/object/include/rttiDataView.h"
#include "base/object/include/dataViewNative.h"

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
        RTTI_CATEGORY("Material hierarchy");
        RTTI_PROPERTY(m_baseMaterial).editable("Base material");
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
                    onPropertyChanged(name.view());
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
            onPropertyChanged("baseMaterial");
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

        if (path == "baseMaterial"_id)
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

    bool MaterialInstance::hasParameterOverride(const base::StringID name) const
    {
        for (const auto& paramInfo : m_parameters)
            if (paramInfo.name == name && !paramInfo.value.empty())
                return true;

        return false;
    }

    bool MaterialInstance::resetParameterOverride(const base::StringID name)
    {
        for (uint32_t i = 0; i < m_parameters.size(); ++i)
        {
            if (m_parameters[i].name == name)
            {
                m_parameters.erase(i);
                onPropertyChanged(name.view());
                return true;
            }
        }

        return false;
    }

    base::DataViewResult MaterialInstance::readDataView(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        base::StringView<char> originalViewPath = viewPath;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* materialTemplate = resolveTemplate())
            {
                if (const auto* paramInfo = materialTemplate->findParameterInfo(base::StringID::Find(propertyName)))
                {
                    // read value from our overrides
                    for (const auto& paramInfo : m_parameters)
                        if (paramInfo.name == propertyName && !paramInfo.value.empty())
                            return paramInfo.value.type()->readDataView(viewPath, paramInfo.value.data(), targetData, targetType);

                    // read default value from base material
                    if (const auto baseMaterial = m_baseMaterial.acquire())
                        return baseMaterial->readDataView(originalViewPath, targetData, targetType); // NOTE orignal path used
                }
            }            
        }

        return TBaseClass::readDataView(originalViewPath, targetData, targetType);
    }

    base::DataViewResult MaterialInstance::writeDataView(base::StringView<char> viewPath, const void* sourceData, base::Type sourceType)
    {
        const auto originalPath = viewPath;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            // find base type info
            if (const auto* materialTemplate = resolveTemplate())
            {
                if (const auto* paramInfo = materialTemplate->findParameterInfo(base::StringID(propertyName)))
                {
                    // ALWAYS write to compatible type
                    auto value = paramInfo->defaultValue;
                    if (const auto ret = HasError(value.type()->writeDataView(viewPath, value.data(), sourceData, sourceType)))
                        return ret; // writing data to type container failed - something is wrong with the data

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
                    return base::DataViewResultCode::OK;
                }
            }
        }

        return TBaseClass::writeDataView(originalPath, sourceData, sourceType);
    }

    base::DataViewResult MaterialInstance::describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        base::StringView<char> originalViewPath = viewPath;

        if (viewPath.empty())
        {
            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
            {
                if (const auto* materialTemplate = resolveTemplate())
                    materialTemplate->listParameters(outInfo);
            }
        }
        else
        {
            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (const auto* materialTemplate = resolveTemplate())
                {
                    if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::CheckIfResetable))
                        if (hasParameterOverride(base::StringID(propertyName)))
                                outInfo.flags |= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;

                    const auto ret = materialTemplate->describeParameterView(propertyName, viewPath, outInfo);
                    if (ret.code != base::DataViewResultCode::ErrorUnknownProperty)
                        return ret;
                }
            }
        }

        return TBaseClass::describeDataView(originalViewPath, outInfo);
    }

    bool MaterialInstance::readParameterDefaultValue(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        auto originalPath = viewPath;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
            if (auto temp = resolveTemplate())
                if (nullptr != temp->findParameterInfo(base::StringID::Find(propertyName)))
                    if (auto base = baseMaterial().acquire())
                        return base->readDataView(originalPath, targetData, targetType).valid();

        return false;
    }

    //---

    class MaterialInstanceDataView : public base::DataViewNative
    {
    public:
        MaterialInstanceDataView(MaterialInstance* mi)
            : DataViewNative(mi)
            , m_material(mi)
        {}

        virtual base::DataViewResult readDefaultDataView(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
        {
            const auto originalPath = viewPath;

            if (!m_material)
                return base::DataViewResultCode::ErrorNullObject;

            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (auto temp = m_material->resolveTemplate())
                {
                    if (nullptr != temp->findParameterInfo(base::StringID::Find(propertyName)))
                    {
                        if (m_material->readParameterDefaultValue(originalPath, targetData, targetType))
                            return base::DataViewResultCode::OK;
                    }
                }
            }

            return DataViewNative::readDataView(originalPath, targetData, targetType);
        }

        virtual base::DataViewResult resetToDefaultValue(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
        {
            if (!m_material)
                return base::DataViewResultCode::ErrorNullObject;

            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (viewPath.empty())
                {
                    if (m_material->resetParameterOverride(base::StringID::Find(propertyName)))
                        return base::DataViewResultCode::OK;
                    return base::DataViewResultCode::ErrorIllegalOperation;
                }
            }

            return base::DataViewNative::resetToDefaultValue(viewPath, targetData, targetType);
        }

        virtual bool checkIfCurrentlyADefaultValue(base::StringView<char> viewPath) const
        {
            if (!m_material)
                return false;

            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
                if (viewPath.empty())
                    return !m_material->hasParameterOverride(base::StringID::Find(propertyName));

            return base::DataViewNative::checkIfCurrentlyADefaultValue(viewPath);
        }

    private:
        MaterialInstance* m_material;
    };

    base::DataViewPtr MaterialInstance::createDataView() const
    {
        return base::CreateSharedPtr<MaterialInstanceDataView>(const_cast<MaterialInstance*>(this));
    }

    //---

} // rendering