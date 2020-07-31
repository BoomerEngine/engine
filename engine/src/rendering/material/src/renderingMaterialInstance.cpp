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

#include "base/resource/include/resourceFactory.h"
#include "base/object/include/rttiDataView.h"
#include "base/object/include/dataViewNative.h"
#include "base/resource/include/resourceTags.h"

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
        RTTI_PROPERTY(m_parameters);
        RTTI_PROPERTY(m_imported);
        RTTI_CATEGORY("Material hierarchy");
        RTTI_PROPERTY(m_baseMaterial).editable("Base material");
    RTTI_END_TYPE();

    static const base::StringID BASE_MATERIAL_NAME = "baseMaterial"_id;

    MaterialInstance::MaterialInstance()
    {
        if (!base::IsDefaultObjectCreation())
        {
            createMaterialProxy();
        }
    }

    MaterialInstance::MaterialInstance(const MaterialRef& baseMaterialRef)
    {
        baseMaterial(baseMaterialRef);
        createMaterialProxy();
    }

    MaterialInstance::MaterialInstance(const MaterialRef& baseMaterialRef, base::Array<MaterialInstanceParam>&& parameters, MaterialInstance* importedMaterialBase)
    {
        if (importedMaterialBase)
        {
            DEBUG_CHECK_EX(importedMaterialBase->parent() == nullptr, "Imported material is already part of something");

            m_imported = AddRef(importedMaterialBase);
            m_imported->parent(this);

            m_parameters = m_imported->m_parameters;

            for (auto& param : parameters)
                writeParameterInternal(std::move(param));

            parameters.reset();
        }
        else
        {
            m_parameters = std::move(parameters);
        }           

        baseMaterial(baseMaterialRef);
        createMaterialProxy();
    }

    MaterialInstance::~MaterialInstance()
    {
    }

    static base::res::StaticResource<MaterialTemplate> resFallbackMaterial("/engine/materials/fallback.v4mg");

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

    bool MaterialInstance::resetParameter(base::StringID name)
    {
        bool reset = false;

        if (m_imported && name == BASE_MATERIAL_NAME)
        {
            baseMaterial(m_imported->baseMaterial());
            return true;
        }

        auto* params = m_parameters.typedData();
        for (auto i : m_parameters.indexRange())
        {
            if (params[i].name == name)
            {
                // if we were imported from somewhere than "resetting to base" means "reset to the value at import"
                bool shouldErase = true;
                if (m_imported)
                {
                    if (const auto* improtedValue = m_imported->findParameterInternal(name))
                    {
                        params[i].value = improtedValue->value;
                        shouldErase = false;
                    }
                }

                if (shouldErase)
                    m_parameters.eraseUnordered(i);

                onPropertyChanged(name.view());
                reset = true;
                break;
            }
        }

        return reset;
    }

    MaterialInstanceParam* MaterialInstance::findParameterInternal(base::StringID name)
    {
        for (auto& param : m_parameters)
            if (param.name == name)
                return &param;
        return nullptr;
    }

    const MaterialInstanceParam* MaterialInstance::findParameterInternal(base::StringID name) const
    {
        for (const auto& param : m_parameters)
            if (param.name == name)
                return &param;
        return nullptr;
    }

    void MaterialInstance::writeParameterInternal(base::StringID name, base::Variant&& value)
    {
        for (auto& param : m_parameters)
        {
            if (param.name == name)
            {
                param.value = std::move(value);
                return;
            }
        }

        auto& entry = m_parameters.emplaceBack();
        entry.name = name;
        entry.value = std::move(value);
    }

    void MaterialInstance::writeParameterInternal(MaterialInstanceParam&& data)
    {
        for (auto& param : m_parameters)
        {
            if (param.name == data.name)
            {
                param.value = std::move(data.value);
                return;
            }
        }

        m_parameters.emplaceBack(std::move(data));
    }

    bool MaterialInstance::writeParameter(base::StringID name, const void* data, base::Type type, bool refresh /*= true*/)
    {
        // allow to change the base material via the same interface (for completeness)
        if (name == BASE_MATERIAL_NAME)
        {
            MaterialRef materialRef;
            if (!base::rtti::ConvertData(data, type, &materialRef, TYPE_OF(materialRef)))
                return false;

            baseMaterial(materialRef);
            return true;
        }

        // update existing parameter
        if (auto* paramInfo = findParameterInternal(name))
        {
            // write new value, can fail is the value is not compatible
            if (paramInfo->value.set(data, type))
            {
                onPropertyChanged(name.view());
                return true;
            }
        }

        // we don't know about this parameter, add it only if it's in the base material
        if (auto materialTemplate = resolveTemplate())
        {
            if (const auto* paramInfo = materialTemplate->findParameterInfo(name))
            {
                auto value = paramInfo->defaultValue;
                if (base::rtti::ConvertData(data, type, value.data(), value.type()))
                {
                    writeParameterInternal(name, std::move(value));
                    return true;
                }
            }
        }

        return false;
    }

    const void* MaterialInstance::findParameterDataInternal(base::StringID name, base::Type& outType) const
    {
        if (const auto* param = findParameterInternal(name))
        {
            outType = param->value.type();
            return param->value.data();
        }

        if (auto base = m_baseMaterial.acquire())
            return base->findParameterDataInternal(name, outType);

        return nullptr;
    }

    bool MaterialInstance::readParameter(base::StringID name, void* data, base::Type type) const
    {
        // allow to read the "baseMaterial" via the same interface, for completeness
        if (name == BASE_MATERIAL_NAME)
            return base::rtti::ConvertData(&m_baseMaterial, TYPE_OF(m_baseMaterial), data, type);

        // find data
        base::Type paramType;
        if (const auto* paramData = findParameterDataInternal(name, paramType))
            return base::rtti::ConvertData(paramData, paramType, data, type);
        
        return false;
    }

    ///---

    void MaterialInstance::removeAllParameters()
    {
        if (!m_parameters.empty())
        {
            auto oldParams = std::move(m_parameters);

            if (m_imported)
                m_parameters = m_imported->parameters();

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

            // HARD-drop all parameters that have incompatible types (this should be rare)
            // ie. in previous base DiffuseScale was "float" but in current base is "Color"
            if (auto materialTemplate = resolveTemplate())
            {
                for (const auto& templateParamInfo : materialTemplate->parameters())
                {
                    if (auto* paramInfo = findParameterInternal(templateParamInfo.name))
                    {
                        if (paramInfo->value.type() != templateParamInfo.type)
                        {
                            TRACE_INFO("Changed type for parameter '{}' from '{}' to '{}', dropping it", templateParamInfo.name, paramInfo->value.type(), templateParamInfo.type);
                            m_parameters.eraseUnordered(paramInfo - m_parameters.typedData());
                        }
                    }
                }
            }

            onPropertyChanged(BASE_MATERIAL_NAME.view());
        }
    }

    void MaterialInstance::onPostLoad()
    {
        TBaseClass::onPostLoad();

        for (int i : m_parameters.indexRange().reversed())
            if (m_parameters[i].value.type() == nullptr)
                m_parameters.eraseUnordered(i);

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
        postEvent(base::EVENT_OBJECT_STRUCTURE_CHANGED);
        createMaterialProxy();
        TBaseClass::notifyBaseMaterialChanged();
    }

    void MaterialInstance::onPropertyChanged(base::StringView<char> path)
    {
        auto orgPropName = path;

        if (path == BASE_MATERIAL_NAME.view())
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

    bool MaterialInstance::checkParameterOverride(base::StringID name) const
    {
        // in general the "base material" is not resetable...
        if (name == BASE_MATERIAL_NAME)
        {
            // ... but in the case we got imported we do have a base value for this parameter
            if (m_imported)
                return m_imported->baseMaterial() != m_baseMaterial;
            return true;// m_baseMaterial != nullptr;
        }

        if (const auto* localParam = findParameterInternal(name))
        {
            // if we were imported than we are truly overridden only if the local value here differs from the one from the import
            if (m_imported)
            {
                if (const auto* importedParam = m_imported->findParameterInternal(name))
                {
                    // watch out for type change - can happen if we use different base material than original import and the parameter there is redefined to different type
                    // real world example. DiffuseScale was float in std_pbr_simple, but in std_pbr is a Color
                    if (importedParam->value.type() == localParam->value.type())
                    {
                        if (localParam->value.type()->compare(importedParam->value.data(), localParam->value.data()))
                            return false;
                    }
                }
            }

            // since parameter exists assume it's overridden
            return true;
        }

        // it doesn't look like anything to me
        return false;
    }

    base::DataViewResult MaterialInstance::readDataView(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        base::StringView<char> originalViewPath = viewPath;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            // try to read as a parameter name
            base::Type paramType;
            const auto parameterName = base::StringID::Find(propertyName); // do not allocate invalid name
            if (const auto* paramData = findParameterDataInternal(parameterName, paramType))
                return paramType->readDataView(viewPath, paramData, targetData, targetType);
        }

        return TBaseClass::readDataView(originalViewPath, targetData, targetType);
    }

    base::DataViewResult MaterialInstance::writeDataView(base::StringView<char> viewPath, const void* sourceData, base::Type sourceType)
    {
        const auto originalPath = viewPath;

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            // we can only write/create parameters that exist in the material TEMPLATE
            if (const auto* materialTemplate = resolveTemplate())
            {
                const auto paramName = base::StringID::Find(propertyName);
                if (const auto* paramInfo = materialTemplate->findParameterInfo(paramName))
                {
                    // ALWAYS write to compatible type to avoid confusion
                    auto value = paramInfo->defaultValue;
                    if (const auto ret = HasError(value.type()->writeDataView(viewPath, value.data(), sourceData, sourceType)))
                        return ret; // writing data to type container failed - something is wrong with the data

                    // add or replace the value
                    writeParameterInternal(paramName, std::move(value));

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
                    {
                        if (checkParameterOverride(base::StringID(propertyName)))
                            outInfo.flags |= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;

                        // HACK: there's no default "baseMaterial" on non-imported instances
                        if (propertyName == BASE_MATERIAL_NAME.view() && !m_imported)
                            outInfo.flags -= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;
                    }

                    const auto ret = materialTemplate->describeParameterView(propertyName, viewPath, outInfo);
                    if (ret.code != base::DataViewResultCode::ErrorUnknownProperty)
                        return ret;
                }
            }
        }

        return TBaseClass::describeDataView(originalViewPath, outInfo);
    }

    const void* MaterialInstance::findBaseParameterDataInternal(base::StringID name, base::Type& outType) const
    {
        // if we were imported then use the imported values as a base FIRST
        if (m_imported)
        {
            if (const auto* importParamInfo = m_imported->findParameterInternal(name))
            {
                outType = importParamInfo->value.type();
                return importParamInfo->value.data();
            }
        }

        // check the base as normal
        if (auto base = m_baseMaterial.acquire())
            return base->findParameterDataInternal(name, outType); // NOTE: we ask base for it's value, not it's base value

        return nullptr;
    }

    bool MaterialInstance::readBaseParameter(base::StringID name, void* data, base::Type type) const
    {
        // small "hack" for imported materials to report the base material from the imported template
        if (m_imported && name == BASE_MATERIAL_NAME)
            return base::rtti::ConvertData(&m_imported->m_baseMaterial, TYPE_OF(m_baseMaterial), data, type);

        // find the data holder
        base::Type paramType;
        if (const auto* paramValue = findBaseParameterDataInternal(name, paramType))
            return base::rtti::ConvertData(paramValue, paramType, data, type);

        // no base value to read
        // TODO: consider returning default value for the type ?
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
                const auto paramName = base::StringID::Find(propertyName); // avoid allocating BS names

                base::Type paramType;
                if (const auto* paramValue = m_material->findBaseParameterDataInternal(paramName, paramType))
                    return paramType->readDataView(viewPath, paramValue, targetData, targetType); // just follow with the read
            }

            return DataViewNative::readDefaultDataView(originalPath, targetData, targetType);
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
                    if (m_material->resetParameter(base::StringID::Find(propertyName)))
                        return base::DataViewResultCode::OK;
                    return base::DataViewResultCode::ErrorIllegalOperation; // we can't reset sub-component of a value
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
                    return !m_material->checkParameterOverride(base::StringID::Find(propertyName));

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