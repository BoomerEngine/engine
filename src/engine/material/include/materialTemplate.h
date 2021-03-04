/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "material.h"
#include "runtimeLayout.h"
#include "runtimeTechnique.h"

#include "core/resource/include/resource.h"
#include "core/system/include/spinLock.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// UI definition for parameters
struct ENGINE_MATERIAL_API MaterialTemplateParamInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialTemplateParamInfo);

    StringID name;
    Variant defaultValue;
    MaterialDataLayoutParameterType parameterType;
};

///---

/// Dynamic material compiler is asked to compile a material technique if it's missing
/// Usually it carries all data within (like the copy of the material graph, etc) that are required to produce a shader
class ENGINE_MATERIAL_API IMaterialTemplateDynamicCompiler : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMaterialTemplateDynamicCompiler, IObject);

public:
    virtual ~IMaterialTemplateDynamicCompiler();

    /// query material render states for given material setup
    virtual void evalRenderStates(const IMaterial& setup, MaterialRenderState& outRenderStates) const = 0;

    /// compile shaders for given material technique, when compiled the technique will be updated with new "compiled state"
    virtual void requestTechniqueComplation(StringView contextName, MaterialTechnique* technique) = 0;
};

///---

/// precompiled material technique
class ENGINE_MATERIAL_API MaterialPrecompiledStaticTechnique
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialPrecompiledStaticTechnique);

    MaterialCompilationSetup setup;
    gpu::ShaderDataPtr shader;

    MaterialPrecompiledStaticTechnique();
    ~MaterialPrecompiledStaticTechnique();
};

///---

// material rendering flags
struct ENGINE_MATERIAL_API MaterialRenderState
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialRenderState);

public:
    bool hasVertexAnimation = false; // vertices are moving
    bool hasPixelDiscard = false; // material has masking
    bool hasTransparency = false;  // material has transparency
    bool hasLighting = false; // material has lighting
    bool hasPixelReadback = false; // material is reading back the color (fake glass)

    MaterialRenderState();
};

///---

/// parameter of the material template
class ENGINE_MATERIAL_API IMaterialTemplateParam : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMaterialTemplateParam, IObject);

public:
    virtual ~IMaterialTemplateParam();

    //--

    /// change name
    void rename(StringID name);

    //--

    /// get name of the parameter
    INLINE StringID name() const { return m_name; }

    // get parameter category
    INLINE StringID category() const { return m_category; }

    // editor hint
    INLINE const StringBuf& hint() const { return m_hint; }

    //--

    // get the type
    virtual MaterialDataLayoutParameterType queryType() const = 0;

    // get data type of the parameter
    virtual Type queryDataType() const = 0;

    // get default value of the parameter
    virtual const void* queryDefaultValue() const = 0;

private:
    StringID m_name;
    StringID m_category;

    StringBuf m_hint;
};

///---

/// a generalized template for material - usually implemented by the means of a graph
/// NOTE: template is considered immutable at runtime (so no editor) - once loaded it will not change
class ENGINE_MATERIAL_API MaterialTemplate : public IMaterial
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplate, IMaterial);

public:
    MaterialTemplate();
    virtual ~MaterialTemplate();

    //----

    // list of material parameters
    INLINE const Array<MaterialTemplateParamPtr>& parameters() const { return m_parameters; }

    //----

    // IMaterial interface
    virtual MaterialDataProxyPtr dataProxy() const override final;
	virtual MaterialTemplateProxyPtr templateProxy() const override final;

	virtual const MaterialTemplate* resolveTemplate() const override final;
    virtual bool readParameter(StringID name, void* data, Type type) const override final;

    //--

    // find parameter by name
    const IMaterialTemplateParam* findParameter(StringID name) const;

	///---

protected:
    virtual void onPostLoad() override;

    //--

    Array<MaterialTemplateParamPtr> m_parameters;

    //--

    MaterialDataProxyPtr m_dataProxy;
    void createDataProxy();

	MaterialTemplateProxyPtr m_templateProxy;
	void createTemplateProxy();

    //--

    virtual RefPtr<IMaterialTemplateDynamicCompiler> queryDynamicCompiler() const = 0;

    StringBuf m_contextPath;

    //--
};

///---

END_BOOMER_NAMESPACE()
