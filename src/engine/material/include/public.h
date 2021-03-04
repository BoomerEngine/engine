/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_material_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

enum class MaterialDataLayoutType : uint8_t
{
	ResourceBindingPoints, // textures are bound to discrete binding points constants are in a buffer
	Bindless, // textures ID and constants are in the same buffer
};

struct MaterialInstanceParam;
struct MaterialRenderState;

class IMaterial;
typedef RefPtr<IMaterial> MaterialPtr;
typedef res::Ref<IMaterial> MaterialRef;
typedef res::AsyncRef<IMaterial> MaterialAsyncRef;

class IMaterialCompiledParameters;
typedef RefPtr<IMaterialCompiledParameters> MaterialCompiledParametersPtr;

class IMaterialCompiledTechnique;
typedef RefPtr<IMaterialCompiledTechnique> MaterialCompiledTechniquePtr;

class MaterialInstance;
typedef RefPtr<MaterialInstance> MaterialInstancePtr;
typedef res::Ref<MaterialInstance> MaterialInstanceRef;

class MaterialTemplate;
typedef RefPtr<MaterialTemplate> MaterialTemplatePtr;
typedef RefWeakPtr<MaterialTemplate> MaterialTemplateWeakPtr;
typedef res::Ref<MaterialTemplate> MaterialTemplateRef;

class MaterialTemplateProxy;
typedef RefPtr<MaterialTemplateProxy> MaterialTemplateProxyPtr;

class MaterialTechnique;
typedef RefPtr<MaterialTechnique> MaterialTechniquePtr;

class IMaterialTemplateDynamicCompiler;
typedef RefPtr<IMaterialTemplateDynamicCompiler> MaterialTemplateDynamicCompilerPtr;

struct MaterialTechniqueRenderStates;

class MaterialDataLayout;
typedef RefPtr<MaterialDataLayout> MaterialDataLayoutPtr;

class MaterialDataProxy;
typedef RefPtr<MaterialDataProxy> MaterialDataProxyPtr;

class MaterialDataView;
typedef RefPtr<MaterialDataView> MaterialDataViewPtr;

class IMaterialTemplateParam;
typedef RefPtr<IMaterialTemplateParam> MaterialTemplateParamPtr;

typedef uint16_t MaterialDataLayoutID;
typedef uint16_t MaterialDataProxyID;

typedef uint16_t MaterialBindlessTextureID;

///---

enum class MaterialPass : uint8_t
{
    DepthPrepass, // depth pre pass, masked, outputs velocity buffer as well
    WireframeSolid, // solid wireframe view, fast and nice
    WireframePassThrough, // transparent wireframe view, slow and aliased
    ConstantColor, // output constant object color
    SelectionFragments, // selection fragment gathering - may be masked, uses specialized shader
    Forward, // classic forward rendering with full lighting
    ForwardTransparent, // classic forward rendering with full lighting
    ShadowDepth, // depth for shadow rendering
    MaterialDebug, // material debugging

    MAX,
};

///---

enum class MeshVertexFormat : uint8_t
{
	PositionOnly, // position only vertex stream, limited use as shadow mesh
	Static, // static vertex, most common - UV + tangent space
	StaticEx, // static vertex with color and second UV
	Skinned4, // 4 bone skinning
	Skinned4Ex, // 4 bone skinning with color and extra UV

	MAX,
};

///---

extern ENGINE_MATERIAL_API StringID MeshVertexFormatBindPointName(MeshVertexFormat format);

///---
    
struct ENGINE_MATERIAL_API MaterialCompilationSetup
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialCompilationSetup);

    uint16_t staticSwitches = 0;

    MaterialPass pass = MaterialPass::Forward;
	MeshVertexFormat vertexFormat = MeshVertexFormat::Static;

	bool bindlessTextures = false;
	bool meshletsVertices = false;

    bool msaa = false;

    uint32_t key() const;
    void print(IFormatStream& f) const;
};

///---

END_BOOMER_NAMESPACE()

