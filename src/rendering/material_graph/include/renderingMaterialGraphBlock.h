/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "renderingMaterialCode.h"
#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphSocket.h" 

namespace rendering
{
    //--

    /// a block in the material graph
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlock : public base::graph::Block
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock, base::graph::Block);

    public:
        MaterialGraphBlock();
        virtual ~MaterialGraphBlock();

        /// does this block generate different code for each output (most blocks don't)
        virtual bool differentCodeForEachOutput() const { return false; }

        /// compile this material block for given output, usually "out" is the only output
        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const = 0;
    };

    //--

    /// block output socket for numerical value
    class RENDERING_MATERIAL_GRAPH_API MaterialOutputSocket : public base::graph::BlockSocketStyleInfo
    {
    public:
        MaterialOutputSocket()
        {
            m_direction = base::graph::SocketDirection::Output;
            m_placement = base::graph::SocketPlacement::Right;
            m_tags.emplaceBack("MaterialGraphValue"_id);
            m_multiconnection = true;
        }
    };

    //--

    /// block input socket for numerical value
    class RENDERING_MATERIAL_GRAPH_API MaterialInputSocket : public base::graph::BlockSocketStyleInfo
    {
    public:
        MaterialInputSocket()
        {
            m_direction = base::graph::SocketDirection::Input;
            m_placement = base::graph::SocketPlacement::Left;
            m_tags.emplaceBack("MaterialGraphValue"_id);
            m_multiconnection = false;
        }
    };

    //--

} // rendering