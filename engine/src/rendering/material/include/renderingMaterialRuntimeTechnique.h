/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

namespace rendering
{
    //---

    /// rendering state setup that the technique expects to have set
    struct RENDERING_MATERIAL_API MaterialTechniqueRenderStates
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialTechniqueRenderStates);

		MaterialTechniqueRenderStates();

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
    struct RENDERING_MATERIAL_API MaterialCompiledTechnique : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE)

    public:
		ShaderDataPtr shader;
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

		/// unique technique ID
		INLINE uint32_t id() const { return m_id; }

        /// get the setup used to create this technique
        INLINE const MaterialCompilationSetup& setup() const { return m_setup; }

        // get the PSO for the material technique
		INLINE const MaterialCompiledTechnique* data() const { return m_data.load(); }

        //--

        // push new rendering shader to the technique
        void pushData(MaterialCompiledTechnique* newState);

        //--

    private:
		MaterialCompilationSetup m_setup;

		std::atomic<MaterialCompiledTechnique*> m_data = nullptr;
		base::Array<MaterialCompiledTechnique*> m_expiredData;

		uint32_t m_id = 0;
    };

    //---

} // rendering