/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// Uncooked layer in the world, basic building block and source of most entities
/// Layer is composed of a list of node templates
class ENGINE_WORLD_API RawLayer : public IResource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(RawLayer, IResource);

public:
    //--

    static inline StringView FILE_EXTENSION = "xlayer";

    //--

    RawLayer();

    INLINE const Array<RawEntityPtr>& nodes() const { return m_nodes; }

    void setup(const Array<RawEntityPtr>& sourceNodes);

private:
    Array<RawEntityPtr> m_nodes;
};

//---

END_BOOMER_NAMESPACE()
