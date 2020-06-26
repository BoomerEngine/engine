/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"

#include "sceneEditorStructure.h"
#include "sceneEditorHelperObjects.h"
#include "sceneEditorStructureNode.h"

namespace ed
{
    namespace world
    {
        ///---

        RTTI_BEGIN_TYPE_CLASS(IHelperObject);
        RTTI_END_TYPE();

        IHelperObject::IHelperObject(const base::RefPtr<ContentNode>& owner)
            : m_owner(owner)
            //, m_ownerId(owner->documentId())
        {}

        IHelperObject::~IHelperObject()
        {}

        bool IHelperObject::validate()
        {
            return true;
        }

        bool IHelperObject::placement(const base::edit::DocumentObjectID& id, base::AbsoluteTransform& outTransform)
        {
            return false;
        }

        bool IHelperObject::placement(const base::edit::DocumentObjectID& id, const base::AbsoluteTransform& newTransform)
        {
            return false;
        }

        void IHelperObject::handleSelectionChange(const base::edit::DocumentObjectIDSet& oldSelection, const base::edit::DocumentObjectIDSet& newSelection)
        {
        }

        void IHelperObject::handleRendering(const base::edit::DocumentObjectIDSet& selection, rendering::scene::FrameInfo& frame)
        {
        }

        ///---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IHelperObjectHandler);
        RTTI_END_TYPE();

        IHelperObjectHandler::IHelperObjectHandler()
        {
        }

        IHelperObjectHandler::~IHelperObjectHandler()
        {
            reset();
        }

        void IHelperObjectHandler::reset()
        {
            m_nodeMap.clearPtr();
            m_activeNodes.clear();
            m_activeSelection.clear();
        }

        void IHelperObjectHandler::removeExpiredHelperObjects()
        {
            // cleanup helpers that expired
            base::InplaceArray<Node*, 100> toRemove;
            for (auto node  : m_nodeMap.values())
            {
                // remove if node expired
                if (node->m_node.expired())
                {
                    toRemove.pushBack(node);
                    continue;
                }

                // validate
                if (node->m_helperCreated && node->m_helper && !node->m_helper->validate())
                {
                    // create helper
                    if (auto sceneNode = node->m_node.lock())
                    {
                        node->m_helper = createHelperObject(sceneNode);

                        if (node->m_helper)
                            node->m_helper->handleSelectionChange(base::edit::DocumentObjectIDSet(), node->m_selection);
                    }

                    continue;
                }
            }

            // cleanup the nodes
            for (auto node  : toRemove)
            {
                m_activeNodes.remove(node);
                m_nodeMap.remove(node->m_id);
                MemDelete(node);
            }
        }

        const base::RefPtr<IHelperObject> IHelperObjectHandler::findHelperObject(const base::edit::DocumentObjectID& id) const
        {
            /*if (id.isFromChild())
                return findHelperObject(id.resolveChildParentID());*/

            Node* ret = nullptr;
            if (m_nodeMap.find(id, ret))
                return ret->m_helper;

            return nullptr;
        }

        void IHelperObjectHandler::activeSelection(ContentStructure& tree, const base::edit::DocumentObjectIDSet& selection)
        {
            // cleanup
            removeExpiredHelperObjects();

            // only if selection changed
            if (m_activeSelection != selection)
            {
                // set raw selection
                m_activeSelection = selection;

                // collect the nodes from this shit
                base::edit::DocumentObjectIDSet activeNodes;
                base::HashMap<base::edit::DocumentObjectID, base::Array<base::edit::DocumentObjectID>> children;
                /*for (auto &id : selection.ids())
                {
                    if (id.isFromChild())
                    {
                        auto parentId = id.resolveChildParentID();
                        if (parentId.type() == "SceneContentNode"_id)
                        {
                            activeNodes.add(parentId);
                            children[parentId].pushBack(id);
                        }
                    }
                    else if (id.type() == "SceneContentNode"_id)
                    {
                        activeNodes.add(id);
                    }
                }
            
                // create the node container for each node that is active
                m_activeNodes.clear();
                for (auto& nodeId : activeNodes.ids())
                {
                    // create wrapper
                    Node *node = nullptr;
                    if (!m_nodeMap.find(nodeId, node))
                    {
                        node = MemNew(Node);
                        node->id = nodeId;
                        //node->node = tree.objectMap().findObjectOfClass<ContentNode>(nodeId);
                        m_nodeMap[nodeId] = node;
                    }

                    // try only once
                    if (!node->m_helperCreated)
                    {
                        if (auto sceneNode = node->node.lock())
                            node->m_helper = createHelperObject(sceneNode);
                        node->m_helperCreated = true;
                    }

                    // collect selected sub-objects for this node
                    auto oldSelection = std::move(node->m_selection);
                    node->m_selection.clear();
                    for (auto &childId : children[nodeId])
                        node->m_selection.add(childId);

                    // notify about selection change within the helper
                    if (node->m_helper)
                        node->m_helper->handleSelectionChange(oldSelection, node->m_selection);

                    // add to list of active nodes
                    // NOTE: we don't have helpers yet
                    m_activeNodes.pushBack(node);
                }*/
            }
        }

        void IHelperObjectHandler::render(rendering::scene::FrameInfo& frame)
        {
            removeExpiredHelperObjects();

            for (auto node  : m_activeNodes)
            {
                if (node->m_helper)
                    node->m_helper->handleRendering(node->m_selection, frame);
            }
        }

        ///---

    } // mesh
} // ed