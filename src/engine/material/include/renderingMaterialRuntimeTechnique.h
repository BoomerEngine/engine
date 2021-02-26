/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// rendering state setup that the technique expects to have set
struct ENGINE_MATERIAL_API MaterialTechniqueRenderStates
{
	MaterialTechniqueRenderStates();

    bool alphaToCoverage = false;
    bool alphaBlend = false;
    bool twoSided = false;
    bool depthTest = true;
    bool depthWrite = true;
    bool earlyPixelTests = false;
    bool hasVertexOffset = false;
    bool fillLines = false;
};

///---

/// compiler data for material technique
struct ENGINE_MATERIAL_API MaterialCompiledTechnique : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE)

public:
    uint32_t setupKey = 0;
    uint64_t shaderKey = 0;
	gpu::ShaderDataPtr shader;
        
    //--

    static const MaterialCompiledTechnique& EMPTY();
};

///---

/// material technique, can be used to directly render the material using compatible pass and geometry
/// NOTE: a given material technique is compatible only with one specific data layout
class ENGINE_MATERIAL_API MaterialTechnique : public IReferencable
{
public:
    MaterialTechnique(const MaterialCompilationSetup& setup);
    virtual ~MaterialTechnique();

	/// unique technique ID
	INLINE uint32_t id() const { return m_id; }

    /// get the setup used to create this technique
    INLINE const MaterialCompilationSetup& setup() const { return m_setup; }

    // get the PSO for the material technique
	INLINE const gpu::GraphicsPipelineObject* pso() const { return m_data.load(); }

    //--

    // push new rendering shader to the technique
    void pushData(const gpu::ShaderData* newState);

    //--

private:
	MaterialCompilationSetup m_setup;

	std::atomic<gpu::GraphicsPipelineObject*> m_data = nullptr;
	Array<gpu::GraphicsPipelineObjectPtr> m_expiredData;

	uint32_t m_id = 0;
};

//---

END_BOOMER_NAMESPACE()
