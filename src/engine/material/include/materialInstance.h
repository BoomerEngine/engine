/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "material.h"
#include "core/reflection/include/variant.h"

BEGIN_BOOMER_NAMESPACE()

///----

class MaterialInstanceDataView;

///----

/// instanced parameter
struct ENGINE_MATERIAL_API MaterialInstanceParam
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialInstanceParam);

    StringID name;
    Variant value;
};

///----

/// a instance of a material, extends properties specified in the base file
class ENGINE_MATERIAL_API MaterialInstance : public IMaterial
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstance, IMaterial);

public:
    MaterialInstance();
    MaterialInstance(const MaterialRef& baseMaterial); // create instance of base material
    MaterialInstance(const MaterialRef& baseMaterial, Array<MaterialInstanceParam>&& parameters, MaterialInstance* importedMaterialBase = nullptr);
    virtual ~MaterialInstance();

    //---

    /// get the current base material
    INLINE const MaterialRef& baseMaterial() const { return m_baseMaterial; }

    /// get the parameter table
    INLINE const Array<MaterialInstanceParam>& parameters() const { return m_parameters; }

    /// get the original imported material (valid only if we were imported obviously)
    INLINE const MaterialInstancePtr& imported() const { return m_imported; }

    //---

    /// change base material, will recompile the material as everything will change
    void baseMaterial(const MaterialRef& baseMaterial);

    /// remove all parameter overrides from this material
    void removeAllParameters();

    /// create/update material proxy NOW
    void createMaterialProxy();

    //---

    // IMaterial
    virtual MaterialDataProxyPtr dataProxy() const override final;
	virtual MaterialTemplateProxyPtr templateProxy() const override final;
    virtual const MaterialTemplate* resolveTemplate() const override final;

    virtual bool checkParameterOverride(StringID name) const override final;
    virtual bool resetParameter(StringID name) override final;
    virtual bool writeParameter(StringID name, const void* data, Type type, bool refresh = true) override final;
    virtual bool readParameter(StringID name, void* data, Type type) const override final;
    virtual bool readBaseParameter(StringID name, void* data, Type type) const override final;

    virtual const void* findBaseParameterDataInternal(StringID name, Type& dataType) const override;
    virtual const void* findParameterDataInternal(StringID name, Type& dataType) const override;

    // IObject
    virtual DataViewPtr createDataView() const;
    virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const override;
    virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) override;
    virtual DataViewResult describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const override;

    virtual bool onResourceReloading(res::IResource* currentResource, res::IResource* newResource) override;
    virtual void onResourceReloadFinished(res::IResource* currentResource, res::IResource* newResource) override;

    //--

protected:
    virtual void onPostLoad() override;
    virtual void onPropertyChanged(StringView path) override;

    virtual void notifyDataChanged() override;
    virtual void notifyBaseMaterialChanged() override;

    //--

    void writeParameterInternal(MaterialInstanceParam&& data);
    void writeParameterInternal(StringID name, Variant&& value);

    const MaterialInstanceParam* findParameterInternal(StringID name) const;
    MaterialInstanceParam* findParameterInternal(StringID name);


    //--

    MaterialRef m_baseMaterial;
    Array<MaterialInstanceParam> m_parameters;

    MaterialInstancePtr m_imported; // original imported material (for reference), set only if we were imported

private:
    MaterialDataProxyPtr m_dataProxy;

    friend class MaterialInstanceDataView;
};

///----

END_BOOMER_NAMESPACE()
