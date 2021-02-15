/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "base/resource_compiler/include/depotStructure.h"

namespace rendering
{

    //---

    /// compiler material technique
    struct MaterialCompiledTechnique;
    extern MaterialCompiledTechnique* CompileTechnique(const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup);

    //---

    /// a local compiler used to compile a single technique of given material
    class MaterialTechniqueCompiler : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE_COMPILER)

    public:
        MaterialTechniqueCompiler(const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique);

        bool compile() CAN_YIELD; // NOTE: slow

    private:
        MaterialCompilationSetup m_setup;

        MaterialGraphContainerPtr m_graph;
        MaterialTechniquePtr m_technique;

        base::StringBuf m_contextName;    
    };

    //---

} // rendering

