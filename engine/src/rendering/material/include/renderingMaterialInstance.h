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

    /// instanced parameter
    struct RENDERING_MATERIAL_API MaterialInstanceParam
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialInstanceParam);

        base::StringID name;
        base::Variant value;

        TextureRef loadedTexture; // loaded texture reference
    };

    ///----

    /// a instance of a material, extends properties specified in the base file
    class RENDERING_MATERIAL_API MaterialInstance : public IMaterial
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstance, IMaterial);

    public:
        MaterialInstance();
        MaterialInstance(const MaterialRef& baseMaterial); // create instance of base material
        virtual ~MaterialInstance();

        /// get the parameter table
        INLINE const base::Array<MaterialInstanceParam>& parameters() const { return m_parameters; }

        /// get the current base material
        INLINE const MaterialRef& baseMaterial() const { return m_baseMaterial; }

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
        virtual const MaterialTemplate* resolveTemplate() const override final;
        virtual bool resetParameterRaw(base::StringID name) override final;
        virtual bool writeParameterRaw(base::StringID name, const void* data, base::Type type, bool refresh = true) override final;
        virtual bool readParameterRaw(base::StringID name, void* data, base::Type type, bool defaultValueOnly = false) const override final;

        // IObject
        virtual bool readDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, void* targetData, base::Type targetType) const override;
        virtual bool writeDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, const void* sourceData, base::Type sourceType) override;
        virtual bool describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const override;

        virtual bool onResourceReloading(base::res::IResource* currentResource, base::res::IResource* newResource) override;
        virtual void onResourceReloadFinished(base::res::IResource* currentResource, base::res::IResource* newResource) override;

        virtual bool resetBase();
        virtual bool hasParameterOverride(const base::StringID data) const;

        //--

    protected:
        virtual void onPostLoad() override;
        virtual void onPropertyChanged(base::StringView<char> path) override;

        virtual void notifyDataChanged() override;
        virtual void notifyBaseMaterialChanged() override;

        MaterialRef m_baseMaterial;
        base::Array<MaterialInstanceParam> m_parameters;

    private:
        MaterialDataProxyPtr m_dataProxy;
    };

    ///----

} // rendering