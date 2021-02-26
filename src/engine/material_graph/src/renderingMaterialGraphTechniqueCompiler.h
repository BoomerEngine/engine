/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// compiler material technique
struct MaterialCompiledTechnique;
extern MaterialCompiledTechnique* CompileTechnique(const StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup);

//---

/// a local compiler used to compile a single technique of given material
class MaterialTechniqueCompiler : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE_COMPILER)

public:
    MaterialTechniqueCompiler(const StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique);

    bool compile() CAN_YIELD; // NOTE: slow

private:
    MaterialCompilationSetup m_setup;

    MaterialGraphContainerPtr m_graph;
    MaterialTechniquePtr m_technique;

    StringBuf m_contextName;
};

//---

END_BOOMER_NAMESPACE()

