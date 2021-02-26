/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_material_graph_glue.inl"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraph;
typedef RefPtr<MaterialGraph> MaterialGraphPtr;
typedef res::Ref<MaterialGraph> MaterialGraphRef;

class MaterialGraphContainer;
typedef RefPtr<MaterialGraphContainer> MaterialGraphContainerPtr;

class MaterialGraphBlock;
typedef RefPtr<MaterialGraphBlock> MaterialGraphBlockPtr;

class MaterialGraphBlockParameter;
typedef RefPtr<MaterialGraphBlockParameter> MaterialGraphParameterBlockPtr;

class MaterialGraphBlockOutput;

///---


class MaterialStageCompiler;

///---

// #51574A
    
extern const Color::ColorVal COLOR_SOCKET_RED;
extern const Color::ColorVal COLOR_SOCKET_GREEN;
extern const Color::ColorVal COLOR_SOCKET_BLUE;
extern const Color::ColorVal COLOR_SOCKET_ALPHA;
extern const Color::ColorVal COLOR_SOCKET_TEXTURE;
extern const Color::ColorVal COLOR_SOCKET_CUBE;
	
///---

END_BOOMER_NAMESPACE()

