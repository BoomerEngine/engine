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

#include "base/resource/include/resource.h"
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
        virtual void requestTechniqueComplation(base::StringView contextName, MaterialTechnique* technique) = 0;
    };

    ///---

    /// precompiled material technique
    class RENDERING_MATERIAL_API MaterialPrecompiledStaticTechnique
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialPrecompiledStaticTechnique);

        MaterialCompilationSetup setup;
        ShaderDataPtr shader;

        MaterialPrecompiledStaticTechnique();
        ~MaterialPrecompiledStaticTechnique();
    };

    ///---

    // material rendering flags
    struct RENDERING_MATERIAL_API MaterialTemplateMetadata
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialTemplateMetadata);

    public:
        bool hasVertexAnimation = false; // vertices are moving
        bool hasPixelDiscard = false; // material has masking
        bool hasTransparency = false;  // material has transparency
        bool hasLighting = false; // material has lighting
        bool hasPixelReadback = false; // material is reading back the color (fake glass)

        MaterialTemplateMetadata();
    };


    ///---

    /// a generalized template for material - usually implemented by the means of a graph
    /// NOTE: template is considered immutable at runtime (so no editor) - once loaded it will not change
    class RENDERING_MATERIAL_API MaterialTemplate : public IMaterial
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplate, IMaterial);

    public:
        MaterialTemplate();
        virtual ~MaterialTemplate();

        //----

        // IMaterial interface
        virtual MaterialDataProxyPtr dataProxy() const override final;
		virtual MaterialTemplateProxyPtr templateProxy() const override final;

		virtual const MaterialTemplate* resolveTemplate() const override final;
        virtual bool checkParameterOverride(base::StringID name) const override final;
        virtual bool resetParameter(base::StringID name) override final;
        virtual bool writeParameter(base::StringID name, const void* data, base::Type type, bool refresh = true) override final; // NOTE: writing to template is not supported (immutable resources and all...)
        virtual bool readParameter(base::StringID name, void* data, base::Type type) const override final;
        virtual bool readBaseParameter(base::StringID name, void* data, base::Type type) const override final;

        virtual const void* findBaseParameterDataInternal(base::StringID name, base::Type& outType) const override final;

        // IObject - extension of object property model that allows to see the template parameters
        virtual base::DataViewResult readDataView(base::StringView viewPath, void* targetData, base::Type targetType) const override;
        virtual base::DataViewResult describeDataView(base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const override;

        ///---

        // list material parameters (for data view)
        virtual void listParameters(base::rtti::DataViewInfo& outInfo) const = 0;

        // find info about material parameters
        virtual bool queryParameterInfo(base::StringID name, MaterialTemplateParamInfo& outInfo) const = 0;

        /// get the material template metadata
        virtual void queryMatadata(MaterialTemplateMetadata& outMetadata) const = 0;

        /// get list of template parameters
        virtual void queryAllParameterInfos(base::Array<MaterialTemplateParamInfo>& outParams) const = 0;

        //--

        // describe parameter data
        base::DataViewResult describeParameterView(base::StringView paramName, base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const;

		///---

    protected:
        virtual void onPostLoad() override;

        //--

        MaterialDataProxyPtr m_dataProxy;
        void createDataProxy();

		MaterialTemplateProxyPtr m_templateProxy;
		void createTemplateProxy();

        //--

        virtual base::RefPtr<IMaterialTemplateDynamicCompiler> queryDynamicCompiler() const = 0;

        base::StringBuf m_contextPath;

        //--
    };

    ///---

} // rendering