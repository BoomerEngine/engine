/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "rendering/device/include/renderingImageView.h"
#include "rendering/device/include/renderingParametersView.h"

namespace rendering
{
    //---

    /// rendering state setup that the technique expects to have set
    struct RENDERING_MATERIAL_API MaterialTechniqueRenderStates
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialTechniqueRenderStates);

        MaterialBlendMode blendMode = MaterialBlendMode::Opaque;
        bool alphaToCoverage = false;
        bool twoSided = false;
        bool depthTest = true;
        bool depthWrite = true;
        bool earlyPixelTests = false;
        bool hasVertexOffset = false;
    };

    ///---

    /// compiler data for material technique
    struct RENDERING_MATERIAL_API MaterialCompiledTechnique : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_RENDERING_TECHNIQUE>
    {
        ShaderLibraryPtr shader;
        MaterialTechniqueRenderStates renderStates;

        const MaterialDataLayout* dataLayout = nullptr;

        //--

        static const MaterialCompiledTechnique& EMPTY();
    };

    ///---

    /// material technique, can be used to directly render the material using compatible pass and geometry
    /// NOTE: a given material technique is compatible only with one specific data layout
    class RENDERING_MATERIAL_API MaterialTechnique : public base::IReferencable
    {
    public:
        MaterialTechnique(const MaterialCompilationSetup& setup);
        virtual ~MaterialTechnique();

        /// get the setup used to create this technique
        INLINE const MaterialCompilationSetup& setup() const { return m_setup; }

        //--

        // get the render states for this technique
        INLINE const MaterialCompiledTechnique& state() const
        {
            auto* data = m_state.load();
            return data ? *data : MaterialCompiledTechnique::EMPTY();
        }

        //--

        // push new rendering shader to the technique
        void pushData(MaterialCompiledTechnique* newState);

        //--

    private:
        std::atomic<MaterialCompiledTechnique*> m_state = nullptr;
        //mutable std::atomic<uint32_t> m_used = 0;

        MaterialCompilationSetup m_setup;

        base::Array<MaterialCompiledTechnique*> m_expiredStates;
    };

    //---

} // rendering