/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "code.h"
#include "core/graph/include/graphBlock.h"
#include "core/graph/include/graphSocket.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// a block in the material graph
class ENGINE_MATERIAL_GRAPH_API MaterialGraphBlock : public graph::Block
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock, graph::Block);

public:
    MaterialGraphBlock();
    virtual ~MaterialGraphBlock();

    /// does this block generate different code for each output (most blocks don't)
    virtual bool differentCodeForEachOutput() const { return false; }

    /// compile this material block for given output, usually "out" is the only output
    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const = 0;
};

//--

/// block output socket for numerical value
class ENGINE_MATERIAL_GRAPH_API MaterialOutputSocket : public graph::BlockSocketStyleInfo
{
public:
    MaterialOutputSocket()
    {
        m_direction = graph::SocketDirection::Output;
        m_placement = graph::SocketPlacement::Right;
        m_tags.emplaceBack("MaterialGraphValue"_id);
        m_multiconnection = true;
    }
};

//--

/// block input socket for numerical value
class ENGINE_MATERIAL_GRAPH_API MaterialInputSocket : public graph::BlockSocketStyleInfo
{
public:
    MaterialInputSocket()
    {
        m_direction = graph::SocketDirection::Input;
        m_placement = graph::SocketPlacement::Left;
        m_tags.emplaceBack("MaterialGraphValue"_id);
        m_multiconnection = false;
    }
};

//--

END_BOOMER_NAMESPACE()
