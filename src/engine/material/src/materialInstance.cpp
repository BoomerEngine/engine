/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "materialInstance.h"
#include "materialTemplate.h"
#include "runtimeProxy.h"
#include "runtimeService.h"

#include "engine/texture/include/texture.h"

#include "core/resource/include/resourceFactory.h"
#include "core/object/include/rttiDataView.h"
#include "core/object/include/dataViewNative.h"
#include "core/resource/include/resourceTags.h"
#include "core/object/include/objectGlobalRegistry.h"

BEGIN_BOOMER_NAMESPACE_EX(rtti)

extern CORE_OBJECT_API bool PatchResourceReferences(Type type, void* data, res::IResource* currentResource, res::IResource* newResource);

END_BOOMER_NAMESPACE_EX(rtti)

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialInstanceParam);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(value);
RTTI_END_TYPE();

//----

// factory class for the material instance
class MaterialInstanceFactory : public res::IFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceFactory, res::IFactory);

public:
    virtual res::ResourceHandle createResource() const override final
    {
        return RefNew<MaterialInstance>();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialInstanceFactory);
    RTTI_METADATA(res::FactoryClassMetadata).bindResourceClass<MaterialInstance>();
RTTI_END_TYPE();


///---

RTTI_BEGIN_TYPE_CLASS(MaterialInstance);
    RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4mi");
    RTTI_METADATA(res::ResourceDescriptionMetadata).description("Material Instance");
    RTTI_METADATA(res::ResourceTagColorMetadata).color(0x4c, 0x5b, 0x61);
    RTTI_PROPERTY(m_parameters);
    RTTI_PROPERTY(m_imported);
    RTTI_CATEGORY("Material hierarchy");
    RTTI_PROPERTY(m_baseMaterial).editable("Base material");
RTTI_END_TYPE();

static const StringID BASE_MATERIAL_NAME = "baseMaterial"_id;

MaterialInstance::MaterialInstance()
{
    if (!IsDefaultObjectCreation())
    {
        createMaterialProxy();
    }
}

MaterialInstance::MaterialInstance(const MaterialRef& baseMaterialRef)
{
    baseMaterial(baseMaterialRef);
    createMaterialProxy();
}

MaterialInstance::MaterialInstance(const MaterialRef& baseMaterialRef, Array<MaterialInstanceParam>&& parameters, MaterialInstance* importedMaterialBase)
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

//--

static const void* FindParameterDataInternal(const IMaterial* a, StringID name, Type& outType)
{
    if (const auto* mt = rtti_cast<MaterialTemplate>(a))
    {
        if (const auto* param = mt->findParameter(name))
        {
            outType = param->queryDataType();
            return param->queryDefaultValue();
        }
    }

    else if (const auto* mi = rtti_cast<MaterialInstance>(a))
    {
        for (const auto& param : mi->parameters())
        {
            if (param.name == name)
            {
                outType = param.value.type();
                return param.value.data();
            }
        }

        return FindParameterDataInternal(mi->baseMaterial().load(), name, outType);
    }

    return nullptr;
}

static const void* FindBaseParameterDataInternal(const MaterialInstance* a, StringID name, Type& outType)
{
    // if we were imported then use the imported values as a base FIRST
    if (a->imported())
    {
        for (const auto& param : a->imported()->parameters())
        {
            if (param.name == name)
            {
                outType = param.value.type();
                return param.value.data();
            }
        }
    }

    // check the base as normal
    return FindParameterDataInternal(a->baseMaterial().load(), name, outType);
}

//--

static res::StaticResource<MaterialTemplate> resFallbackMaterial("/engine/materials/fallback.v4mg", true);

MaterialDataProxyPtr MaterialInstance::dataProxy() const
{
    // use existing proxy
    if (m_dataProxy)
        return m_dataProxy;

    // use proxy from base material
    if (auto base = m_baseMaterial.load())
        return base->dataProxy();

    // use fallback proxy
    auto fallbackTemplate = resFallbackMaterial.loadAndGet();
    DEBUG_CHECK_EX(fallbackTemplate, "Failed to load fallback material template, rendering pipeline is seriously broken");
    return fallbackTemplate->dataProxy();
}

MaterialTemplateProxyPtr MaterialInstance::templateProxy() const
{
	if (auto base = m_baseMaterial.load())
		if (auto proxy = base->templateProxy())
			return proxy;

	auto fallbackTemplate = resFallbackMaterial.loadAndGet();
	DEBUG_CHECK_EX(fallbackTemplate, "Failed to load fallback material template, rendering pipeline is seriously broken");
	return fallbackTemplate->templateProxy();
}

const MaterialTemplate* MaterialInstance::resolveTemplate() const
{
    if (auto base = m_baseMaterial.load())
        if (auto baseTemplate = base->resolveTemplate())
            return baseTemplate;

    auto fallbackTemplate = resFallbackMaterial.loadAndGet();
    DEBUG_CHECK_EX(fallbackTemplate, "Failed to load fallback material template, rendering pipeline is seriously broken");
    return fallbackTemplate;
}

bool MaterialInstance::resetParameter(StringID name)
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

MaterialInstanceParam* MaterialInstance::findParameterInternal(StringID name)
{
    for (auto& param : m_parameters)
        if (param.name == name)
            return &param;
    return nullptr;
}

const MaterialInstanceParam* MaterialInstance::findParameterInternal(StringID name) const
{
    for (const auto& param : m_parameters)
        if (param.name == name)
            return &param;
    return nullptr;
}

void MaterialInstance::writeParameterInternal(StringID name, Variant&& value)
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

bool MaterialInstance::writeParameter(StringID name, const void* data, Type type, bool refresh /*= true*/)
{
    // allow to change the base material via the same interface (for completeness)
    if (name == BASE_MATERIAL_NAME)
    {
        MaterialRef materialRef;
        if (!rtti::ConvertData(data, type, &materialRef, TYPE_OF(materialRef)))
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
        if (const auto* param = materialTemplate->findParameter(name))
        {
            Variant value(param->queryDataType()); // create value holder of type compatible with the template
            if (rtti::ConvertData(data, type, value.data(), value.type()))
            {
                writeParameterInternal(name, std::move(value));
                return true;
            }
        }
    }

    return false;
}

bool MaterialInstance::readParameter(StringID name, void* data, Type type) const
{
    // allow to read the "baseMaterial" via the same interface, for completeness
    if (name == BASE_MATERIAL_NAME)
        return rtti::ConvertData(&m_baseMaterial, TYPE_OF(m_baseMaterial), data, type);

    // find data
    Type paramType;
    if (const auto* paramData = FindParameterDataInternal(this, name, paramType))
        return rtti::ConvertData(paramData, paramType, data, type);
        
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
    auto prevBase = m_baseMaterial.resource();
     
    m_baseMaterial = baseMaterial;

    if (prevBase != m_baseMaterial.resource())
    {
        // HARD-drop all parameters that have incompatible types (this should be rare)
        // ie. in previous base DiffuseScale was "float" but in current base is "Color"
        if (auto materialTemplate = resolveTemplate())
        {
            for (const auto& templateParamInfo : materialTemplate->parameters())
            {
                if (auto* paramInfo = findParameterInternal(templateParamInfo->name()))
                {
                    if (paramInfo->value.type() != templateParamInfo->queryDataType())
                    {
                        //TRACE_INFO("Changed type for parameter '{}' from '{}' to '{}', dropping it", templateParamInfo.name, paramInfo->value.type(), templateParamInfo.type);
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

    changeTrackedMaterial(m_baseMaterial.load());
    createMaterialProxy();
}

static bool IsBasedOnMaterial(const MaterialInstance* a, const IMaterial* base)
{
    while (a)
    {
        if (a == base)
            return true;
        a = rtti_cast<MaterialInstance>(a->baseMaterial().load());
    }

    return false;
}

void MaterialInstance::notifyDataChanged()
{
    createMaterialProxy();
    //TBaseClass::notifyDataChanged();

    ObjectGlobalRegistry::GetInstance().iterateAllObjects([this](IObject* obj)
        {
            if (auto* mi = rtti_cast<MaterialInstance>(obj))
            {
                if (mi != this && IsBasedOnMaterial(mi, this))
                {
                    mi->notifyDataChanged();
                }
            }
            return false;
        });
}

void MaterialInstance::notifyBaseMaterialChanged()
{
    postEvent(EVENT_OBJECT_STRUCTURE_CHANGED);
    createMaterialProxy();
    TBaseClass::notifyBaseMaterialChanged();
}

void MaterialInstance::onPropertyChanged(StringView path)
{
    auto orgPropName = path;

    //TRACE_INFO("OnPropertyChange at {}, prop '{}' base {}", this, path, m_baseMaterial.acquire().get());

    if (path == BASE_MATERIAL_NAME.view())
    {
        const auto baseMaterial = m_baseMaterial.load();
        changeTrackedMaterial(baseMaterial);
        notifyBaseMaterialChanged();
    }
    else
    {
        StringView propertyName;
        if (rtti::ParsePropertyName(path, propertyName) && !propertyName.empty())
        {
            if (const auto* baseTemplate = resolveTemplate())
            {
                if (const auto* param = baseTemplate->findParameter(StringID(propertyName)))
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
    if (const auto baseProxy = templateProxy())
    {
		// create data proxy if template changes
		auto oldProxy = m_dataProxy;
		if (!m_dataProxy || m_dataProxy->templateProxy() != baseProxy)
			m_dataProxy = RefNew<MaterialDataProxy>(baseProxy);

		// push new data
        m_dataProxy->update(*this);

		// if we changed the proxy inform the world about it
		if (oldProxy && oldProxy != m_dataProxy)
            GetService<MaterialService>()->notifyMaterialProxyChanged(oldProxy, m_dataProxy);
    }
}

//--

bool MaterialInstance::onResourceReloading(res::IResource* currentResource, res::IResource* newResource)
{
    if (currentResource->is<MaterialInstance>())
    {
        TRACE_INFO("SeenReloading at {}, cur base {} (reload {}->{}: {})", 
            this, m_baseMaterial.load().get(), currentResource, newResource, newResource->path());
    }

    bool ret = TBaseClass::onResourceReloading(currentResource, newResource);

    for (auto& param : m_parameters)
    {
        if (rtti::PatchResourceReferences(param.value.type(), param.value.data(), currentResource, newResource))
        {
            ret = true;
        }
    }

    return ret;
}

void MaterialInstance::onResourceReloadFinished(res::IResource* currentResource, res::IResource* newResource)
{
    TBaseClass::onResourceReloadFinished(currentResource, newResource);
    createMaterialProxy();
}

bool MaterialInstance::checkParameterOverride(StringID name) const
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

DataViewResult MaterialInstance::readDataView(StringView viewPath, void* targetData, Type targetType) const
{
    StringView originalViewPath = viewPath;

    StringView propertyName;
    if (rtti::ParsePropertyName(viewPath, propertyName))
    {
        // try to read as a parameter name
        Type paramType;
        const auto parameterName = StringID::Find(propertyName); // do not allocate invalid name
        if (const auto* paramData = FindParameterDataInternal(this, parameterName, paramType))
            return paramType->readDataView(viewPath, paramData, targetData, targetType);
    }

    return TBaseClass::readDataView(originalViewPath, targetData, targetType);
}

DataViewResult MaterialInstance::writeDataView(StringView viewPath, const void* sourceData, Type sourceType)
{
    const auto originalPath = viewPath;

    StringView propertyName;
    if (rtti::ParsePropertyName(viewPath, propertyName))
    {
        // we can only write/create parameters that exist in the material TEMPLATE
        if (const auto* materialTemplate = resolveTemplate())
        {
            const auto paramName = StringID::Find(propertyName);

            if (const auto* param = materialTemplate->findParameter(paramName))
            {
                // ALWAYS write to compatible type to avoid confusion
                Variant value(param->queryDataType());
                if (const auto ret = HasError(value.type()->writeDataView(viewPath, value.data(), sourceData, sourceType)))
                    return ret; // writing data to type container failed - something is wrong with the data

                // add or replace the value
                writeParameterInternal(paramName, std::move(value));

                onPropertyChanged(propertyName);
                return DataViewResultCode::OK;
            }
        }
    }

    return TBaseClass::writeDataView(originalPath, sourceData, sourceType);
}

DataViewResult MaterialInstance::describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const
{
    StringView originalViewPath = viewPath;

    if (viewPath.empty())
    {
        if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::MemberList))
        {
            if (const auto* materialTemplate = resolveTemplate())
            {
                for (const auto& param : materialTemplate->parameters())
                {
                    if (param && param->name())
                    {
                        if (auto type = param->queryDataType())
                        {
                            auto& memberInfo = outInfo.members.emplaceBack();
                            memberInfo.name = param->name();
                            memberInfo.category = param->category() ? param->category() : "Generic"_id;
                            memberInfo.type = type;
                        }
                    }
                }
            }

            std::sort(outInfo.members.begin(), outInfo.members.end(), [](const auto& a, const auto& b)
                {
                    return a.name.view() < b.name.view();
                });
        }
    }
    else
    {
        StringView propertyName;
        if (rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* materialTemplate = resolveTemplate())
            {
                if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::CheckIfResetable))
                {
                    if (checkParameterOverride(StringID(propertyName)))
                        outInfo.flags |= rtti::DataViewInfoFlagBit::ResetableToBaseValue;

                    // HACK: there's no default "baseMaterial" on non-imported instances
                    if (propertyName == BASE_MATERIAL_NAME.view() && !m_imported)
                        outInfo.flags -= rtti::DataViewInfoFlagBit::ResetableToBaseValue;
                }

                if (const auto* param = materialTemplate->findParameter(StringID::Find(propertyName)))
                    return param->queryDataType()->describeDataView(viewPath, param->queryDefaultValue(), outInfo);
            }
        }
    }

    return TBaseClass::describeDataView(originalViewPath, outInfo);
}

//--

class MaterialInstanceDataView : public DataViewNative
{
public:
    MaterialInstanceDataView(MaterialInstance* mi)
        : DataViewNative(mi)
        , m_material(mi)
    {}

    virtual DataViewResult readDefaultDataView(StringView viewPath, void* targetData, Type targetType) const
    {
        const auto originalPath = viewPath;

        if (!m_material)
            return DataViewResultCode::ErrorNullObject;

        StringView propertyName;
        if (rtti::ParsePropertyName(viewPath, propertyName))
        {
            const auto paramName = StringID::Find(propertyName); // avoid allocating BS names

            Type paramType;
            if (const auto* paramValue = FindBaseParameterDataInternal(m_material, paramName, paramType))
                return paramType->readDataView(viewPath, paramValue, targetData, targetType); // just follow with the read
        }

        return DataViewNative::readDefaultDataView(originalPath, targetData, targetType);
    }

    virtual DataViewResult resetToDefaultValue(StringView viewPath, void* targetData, Type targetType) const
    {
        if (!m_material)
            return DataViewResultCode::ErrorNullObject;

        StringView propertyName;
        if (rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (viewPath.empty())
            {
                if (m_material->resetParameter(StringID::Find(propertyName)))
                    return DataViewResultCode::OK;
                return DataViewResultCode::ErrorIllegalOperation; // we can't reset sub-component of a value
            }
        }

        return DataViewNative::resetToDefaultValue(viewPath, targetData, targetType);
    }

    virtual bool checkIfCurrentlyADefaultValue(StringView viewPath) const
    {
        if (!m_material)
            return false;

        StringView propertyName;
        if (rtti::ParsePropertyName(viewPath, propertyName))
            if (viewPath.empty())
                return !m_material->checkParameterOverride(StringID::Find(propertyName));

        return DataViewNative::checkIfCurrentlyADefaultValue(viewPath);
    }

private:
    MaterialInstance* m_material;
};

DataViewPtr MaterialInstance::createDataView() const
{
    return RefNew<MaterialInstanceDataView>(const_cast<MaterialInstance*>(this));
}

//---

END_BOOMER_NAMESPACE()
