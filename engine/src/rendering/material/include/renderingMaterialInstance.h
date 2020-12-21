/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "renderingMaterial.h"
#include "base/reflection/include/variant.h"

namespace rendering
{
    ///----

    class MaterialInstanceDataView;

    ///----

    /// instanced parameter
    struct RENDERING_MATERIAL_API MaterialInstanceParam
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialInstanceParam);

        base::StringID name;
        base::Variant value;
    };

    ///----

    /// a instance of a material, extends properties specified in the base file
    class RENDERING_MATERIAL_API MaterialInstance : public IMaterial
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstance, IMaterial);

    public:
        MaterialInstance();
        MaterialInstance(const MaterialRef& baseMaterial); // create instance of base material
        MaterialInstance(const MaterialRef& baseMaterial, base::Array<MaterialInstanceParam>&& parameters, MaterialInstance* importedMaterialBase = nullptr);
        virtual ~MaterialInstance();

        //---

        /// get the current base material
        INLINE const MaterialRef& baseMaterial() const { return m_baseMaterial; }

        /// get the parameter table
        INLINE const base::Array<MaterialInstanceParam>& parameters() const { return m_parameters; }

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

        virtual bool checkParameterOverride(base::StringID name) const override final;
        virtual bool resetParameter(base::StringID name) override final;
        virtual bool writeParameter(base::StringID name, const void* data, base::Type type, bool refresh = true) override final;
        virtual bool readParameter(base::StringID name, void* data, base::Type type) const override final;
        virtual bool readBaseParameter(base::StringID name, void* data, base::Type type) const override final;

        virtual const void* findBaseParameterDataInternal(base::StringID name, base::Type& dataType) const override;
        virtual const void* findParameterDataInternal(base::StringID name, base::Type& dataType) const override;

        // IObject
        virtual base::DataViewPtr createDataView() const;
        virtual base::DataViewResult readDataView(base::StringView viewPath, void* targetData, base::Type targetType) const override;
        virtual base::DataViewResult writeDataView(base::StringView viewPath, const void* sourceData, base::Type sourceType) override;
        virtual base::DataViewResult describeDataView(base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const override;

        virtual bool onResourceReloading(base::res::IResource* currentResource, base::res::IResource* newResource) override;
        virtual void onResourceReloadFinished(base::res::IResource* currentResource, base::res::IResource* newResource) override;

        //--

    protected:
        virtual void onPostLoad() override;
        virtual void onPropertyChanged(base::StringView path) override;

        virtual void notifyDataChanged() override;
        virtual void notifyBaseMaterialChanged() override;

        //--

        void writeParameterInternal(MaterialInstanceParam&& data);
        void writeParameterInternal(base::StringID name, base::Variant&& value);

        const MaterialInstanceParam* findParameterInternal(base::StringID name) const;
        MaterialInstanceParam* findParameterInternal(base::StringID name);


        //--

        MaterialRef m_baseMaterial;
        base::Array<MaterialInstanceParam> m_parameters;

        MaterialInstancePtr m_imported; // original imported material (for reference), set only if we were imported

    private:
        MaterialDataProxyPtr m_dataProxy;

        friend class MaterialInstanceDataView;
    };

    ///----

} // rendering