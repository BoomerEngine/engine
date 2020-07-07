/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "renderingMaterial.h"
#include "renderingMaterialRuntimeLayout.h"
#include "renderingMaterialRuntimeTechnique.h"

#include "base/resources/include/resource.h"
#include "base/system/include/spinLock.h"

namespace rendering
{

    ///---

    /// UI definition for parameters
    struct RENDERING_MATERIAL_API MaterialTemplateParamInfo
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialTemplateParamInfo);

        base::StringID name;
        base::StringID category;
        base::Type type;
        base::Variant defaultValue;
        base::StringBuf automaticTextureSuffix;
        MaterialDataLayoutParameterType parameterType;
    };

    ///---

    /// Dynamic material compiler is asked to compile a material technique if it's missing
    /// Usually it carries all data within (like the copy of the material graph, etc) that are required to produce a shader
    class RENDERING_MATERIAL_API IMaterialTemplateDynamicCompiler : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IMaterialTemplateDynamicCompiler, base::IObject);

    public:
        virtual ~IMaterialTemplateDynamicCompiler();

        /// compile shaders for given material technique, when compiled the technique will be updated with new "compiled state"
        virtual void requestTechniqueComplation(base::StringView<char> contextName, MaterialTechnique* technique) = 0;
    };

    ///---

    /// precompiled material technique
    class RENDERING_MATERIAL_API MaterialPrecompiledStaticTechnique
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialPrecompiledStaticTechnique);

        MaterialCompilationSetup setup;
        MaterialTechniqueRenderStates renderStates;
        ShaderLibraryPtr shader;

        MaterialPrecompiledStaticTechnique();
        ~MaterialPrecompiledStaticTechnique();
    };


    ///---

    /// a generalized template for material - usually implemented by the means of a graph
    /// NOTE: template is considered immutable at runtime (so no editor) - once loaded it will not change
    class RENDERING_MATERIAL_API MaterialTemplate : public IMaterial
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplate, IMaterial);

    public:
        MaterialTemplate();
        MaterialTemplate(base::Array<MaterialTemplateParamInfo>&& params, MaterialSortGroup sortGroup, const MaterialTemplateDynamicCompilerPtr& compiler);
        MaterialTemplate(base::Array<MaterialTemplateParamInfo>&& params, MaterialSortGroup sortGroup, base::Array<MaterialPrecompiledStaticTechnique>&& precompiledTechniques); // soon
        virtual ~MaterialTemplate();

        ///--

        /// get the sort group for the material, usually determined from the type/flags on the output node in the graph
        INLINE MaterialSortGroup sortGroup() const { return m_sortGroup; }

        /// get the data layout for ALL the materials that use this template
        INLINE const MaterialDataLayout* dataLayout() const { return m_dataLayout; }

        /// get list of template parameters
        INLINE const base::Array<MaterialTemplateParamInfo>& parameters() const { return m_parameters; }

        //----

        // IMaterial interface
        virtual MaterialDataProxyPtr dataProxy() const override final;
        virtual const MaterialTemplate* resolveTemplate() const override final;
        virtual bool resetParameterRaw(base::StringID name) override final;
        virtual bool writeParameterRaw(base::StringID name, const void* data, base::Type type, bool refresh = true) override final; // NOTE: writing to template is not supported
        virtual bool readParameterRaw(base::StringID name, void* data, base::Type type, bool defaultValueOnly = false) const override final;

        // IObject - extension of object property model that allows to see the template parameters
        virtual base::DataViewResult readDataView(base::StringView<char> viewPath, void* targetData, base::Type targetType) const override;
        virtual base::DataViewResult describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const override;

        ///---

        // find info about material parameters
        const MaterialTemplateParamInfo* findParameterInfo(base::StringID name) const;

        // list material parameters (for data view)
        void listParameters(base::rtti::DataViewInfo & outInfo) const;

        // describe parameter data
        base::DataViewResult describeParameterView(base::StringView<char> paramName, base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const;

        ///---

        /// find/compile a rendering technique for given rendering settings
        MaterialTechniquePtr fetchTechnique(const MaterialCompilationSetup& setup); 

    protected:
        virtual void onPostLoad() override;

        //--

        MaterialDataProxyPtr m_dataProxy;
        void createDataProxy();

        //--

        MaterialSortGroup m_sortGroup = MaterialSortGroup::Opaque;
        base::Array<MaterialTemplateParamInfo> m_parameters;

        //--

        base::SpinLock m_techniqueMapLock;
        base::HashMap<uint32_t, MaterialTechniquePtr> m_techniqueMap;

        base::Array<MaterialPrecompiledStaticTechnique> m_precompiledTechniques;

        base::RefPtr<IMaterialTemplateDynamicCompiler> m_compiler;

        //--

        void rebuildParameterMap();
        base::HashMap<base::StringID, uint16_t> m_parametersMap; // NOTE: indices in m_parameters

        //--

        void registerLayout();
        const MaterialDataLayout* m_dataLayout = nullptr;

        //--
    };

    ///---

} // rendering