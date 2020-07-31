/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_material_glue.inl"

namespace rendering
{
    struct MaterialSelector;

    class IMaterial;
    typedef base::RefPtr<IMaterial> MaterialPtr;
    typedef base::res::Ref<IMaterial> MaterialRef;
    typedef base::res::AsyncRef<IMaterial> MaterialAsyncRef;

    class IMaterialCompiledParameters;
    typedef base::RefPtr<IMaterialCompiledParameters> MaterialCompiledParametersPtr;

    class IMaterialCompiledTechnique;
    typedef base::RefPtr<IMaterialCompiledTechnique> MaterialCompiledTechniquePtr;

    class MaterialInstance;
    typedef base::RefPtr<MaterialInstance> MaterialInstancePtr;
    typedef base::res::Ref<MaterialInstance> MaterialInstanceRef;

    class MaterialTemplate;
    typedef base::RefPtr<MaterialTemplate> MaterialTemplatePtr;
    typedef base::RefWeakPtr<MaterialTemplate> MaterialTemplateWeakPtr;
    typedef base::res::Ref<MaterialTemplate> MaterialTemplateRef;

    class MaterialTechnique;
    typedef base::RefPtr<MaterialTechnique> MaterialTechniquePtr;

    class IMaterialTemplateDynamicCompiler;
    typedef base::RefPtr<IMaterialTemplateDynamicCompiler> MaterialTemplateDynamicCompilerPtr;

    struct MaterialTechniqueRenderStates;

    class MaterialDataLayout;
    typedef base::RefPtr<MaterialDataLayout> MaterialDataLayoutPtr;

    class MaterialDataProxy;
    typedef base::RefPtr<MaterialDataProxy> MaterialDataProxyPtr;

    class MaterialDataView;
    typedef base::RefPtr<MaterialDataView> MaterialDataViewPtr;

    typedef uint16_t MaterialDataLayoutID;
    typedef uint16_t MaterialDataProxyID;

    ///---

    // blending mode to use for blend able materials, controls the render state
    // NOTE: this makes material NOT leave depth and also makes it rendered in the transparent pass
    enum class MaterialBlendMode : uint8_t
    {
        Opaque, // no blending
        AlphaBlend, // transparent pass, classical alpha blending - PREMULTIPLIED
        Addtive, // transparent pass, additive blending - NOT SORTED
        Refractive, // refraction
    };

    enum class MaterialPass : uint8_t
    {
        DepthPrepass, // depth pre pass, masked, outputs velocity buffer as well
        Wireframe, // wireframe view, can be either solid or wire-only 
        ConstantColor, // output constant object color
        SelectionFragments, // selection fragment gathering - may be masked, uses specialized shader
        Forward, // classic forward rendering with full lighting
        ShadowDepth, // depth for shadow rendering
        MaterialDebug, // material debugging

        MAX,
    };

    enum class MaterialSortGroup : uint8_t
    {
        Opaque, // material is fully opaque, does not use masking
        OpaqueMasked, // material does not use blending but uses discard (masking)
        Transparent, // material requires blending, might or may not use masking
        Refractive, // refractive material that requires readback of the color buffer
    };

    ///---
    
    struct RENDERING_MATERIAL_API MaterialCompilationSetup
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialCompilationSetup);

        MaterialPass pass = MaterialPass::Forward;
        uint8_t vertexFormat = 0;
        uint8_t vertexFetchMode = 0;
        bool msaa = false;

        uint32_t key() const;
        void print(base::IFormatStream& f) const;
    };


    ///---

} // rendering

