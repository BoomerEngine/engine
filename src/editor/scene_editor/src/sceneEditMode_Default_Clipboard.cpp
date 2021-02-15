/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"

#include "sceneContentNodes.h"
#include "sceneContentStructure.h"

#include "sceneEditMode_Default_Clipboard.h"
#include "sceneEditMode_Default.h"

#include "base/resource/include/objectIndirectTemplate.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_CLASS(SceneContentClipboardNode);
        RTTI_PROPERTY(type);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(worldPlacement);
        RTTI_PROPERTY(localPlacement);
        RTTI_PROPERTY(packedEntityData);
        RTTI_PROPERTY(packedComponentData);
    RTTI_END_TYPE();

    SceneContentClipboardNode::SceneContentClipboardNode()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(SceneContentClipboardData);
        RTTI_PROPERTY(type);
        RTTI_PROPERTY(data);
    RTTI_END_TYPE();

    SceneContentClipboardData::SceneContentClipboardData()
    {}

    //--

    static SceneContentClipboardNodePtr BuildClipboardNode(const SceneContentNode* node)
    {
        if (const auto* entityNode = rtti_cast<SceneContentEntityNode>(node))
        {
            auto ret = RefNew<SceneContentClipboardNode>();
            ret->type = SceneContentNodeType::Entity;
            ret->name = entityNode->name();
            ret->worldPlacement = entityNode->cachedLocalToWorldTransform();
            ret->localPlacement = entityNode->localToParent();

            ret->packedEntityData = entityNode->compiledForCopy();
            if (!ret->packedEntityData)
                return nullptr;

            ret->packedEntityData->parent(ret);
            return ret;
        }

        return nullptr;
    }

    SceneContentClipboardDataPtr BuildClipboardDataFromNodes(const Array<SceneContentNodePtr>& nodes)
    {
        // get the root nodes of the selection
        Array<SceneContentNodePtr> roots;
        SceneEditMode_Default::ExtractSelectionRoots(nodes, roots);

        // no roots
        if (roots.empty())
            return nullptr;

        // find common type of the nodes
        auto commonType = SceneContentNodeType::None;
        for (const auto& node : roots)
        {
            if (commonType == SceneContentNodeType::None)
                commonType = node->type();
            else if (commonType != node->type())
                return nullptr; // incompatible selection
        }

        // extract clipboard nodes from matching roots
        auto ret = RefNew<SceneContentClipboardData>();
        ret->type = commonType;

        for (const auto& node : roots)
        {
            if (node->type() == commonType)
            {
                if (auto data = BuildClipboardNode(node))
                {
                    data->parent(ret);
                    ret->data.pushBack(data);
                }
            }
        }

        return ret;
    }

    //--
    
} // ed
