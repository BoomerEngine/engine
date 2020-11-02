/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#include "build.h"
#include "sceneContentStructure.h"
#include "sceneContentNodes.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentStructure);
    RTTI_END_TYPE();

    SceneContentStructure::SceneContentStructure(const game::WorldPtr& previewWorld, SceneContentNode* rootNode)
        : m_root(AddRef(rootNode))
        , m_previewWorld(previewWorld)
    { 
        m_root->bindRootStructure(this);
    }

    SceneContentStructure::~SceneContentStructure()
    {}

    void SceneContentStructure::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        for (const auto& ent : m_nodes)
            ent->handleDebugRender(frame);
    }

    void SceneContentStructure::handleNodeAdded(SceneContentNode* node)
    {
        ASSERT(node);
        ASSERT(node->structure() == this);
        DEBUG_CHECK_RETURN(!m_nodes.contains(node));

        m_nodes.pushBack(AddRef(node));

        if (auto entity = rtti_cast<SceneContentEntityNode>(node))
            m_entities.pushBack(AddRef(entity));
    }

    void SceneContentStructure::handleNodeRemoved(SceneContentNode* node)
    {
        ASSERT(node);
        ASSERT(node->structure() == this);
        DEBUG_CHECK_RETURN(m_nodes.contains(node));

        m_nodes.remove(node);

        if (auto entity = rtti_cast<SceneContentEntityNode>(node))
            m_entities.remove(entity);
    }

    //---

} // ed