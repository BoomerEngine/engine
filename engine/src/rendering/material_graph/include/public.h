/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_material_graph_glue.inl"

namespace rendering
{
    ///---

    class MaterialGraph;
    typedef base::RefPtr<MaterialGraph> MaterialGraphPtr;
    typedef base::res::Ref<MaterialGraph> MaterialGraphRef;

    class MaterialGraphContainer;
    typedef base::RefPtr<MaterialGraphContainer> MaterialGraphContainerPtr;

    class MaterialGraphBlock;
    typedef base::RefPtr<MaterialGraphBlock> MaterialGraphBlockPtr;

    class MaterialGraphBlockParameter;
    typedef base::RefPtr<MaterialGraphBlockParameter> MaterialGraphParameterBlockPtr;

    class MaterialGraphBlockOutput;

    ///---


    class MaterialStageCompiler;

    ///---

    // #51574A
    
    extern const base::Color::ColorVal COLOR_SOCKET_RED;
    extern const base::Color::ColorVal COLOR_SOCKET_GREEN;
    extern const base::Color::ColorVal COLOR_SOCKET_BLUE;
    extern const base::Color::ColorVal COLOR_SOCKET_ALPHA;
    extern const base::Color::ColorVal COLOR_SOCKET_TEXTURE;
    extern const base::Color::ColorVal COLOR_SOCKET_CUBE;

    ///---

} // rendering

