/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructurePrefabRoot.h"

#include "scene/common/include/scenePrefab.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(ContentPrefabRoot);
        RTTI_END_TYPE();

        ContentPrefabRoot::ContentPrefabRoot(const depot::ManagedFilePtr& file)
            : IContentElement(ContentElementType::Prefab)
            , m_fileHandler(file)
        {
            if (m_prefab == file->loadEditableContent<scene::Prefab>())
            {
                syncNodes();
            }
        }

        ContentPrefabRoot::~ContentPrefabRoot()
        {
        }

        const base::StringBuf& ContentPrefabRoot::name() const
        {
            static base::StringBuf theName("world");
            return theName;
        }

        scene::NodePath ContentPrefabRoot::path() const
        {
            return scene::NodePath();
        }

        base::ObjectPtr ContentPrefabRoot::resolveObject(base::ClassType expectedClass) const
        {
            return m_prefab;
        }

        void ContentPrefabRoot::visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const
        {
            if (!func(sharedFromThis()))
                return;

            for (auto& node : m_nodes)
                node->visitStructure(func);
        }

        void ContentPrefabRoot::refreshVisualizationVisibility()
        {

        }

        ContentElementMask ContentPrefabRoot::contentType() const
        {
            ContentElementMask ret;
            ret |= ContentElementBit::Prefab;
            ret |= ContentElementBit::Root;
            ret |= ContentElementBit::NodeContainer;
            ret |= ContentElementBit::CanActivate;
            return ret;
        }

        void ContentPrefabRoot::collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const
        {
            if (isModified())
                outModifiedContent.pushBack(sharedFromThis());
        }

        void ContentPrefabRoot::render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const
        {
            TBaseClass::render(frame, sqDistanceLimit);

            for (auto& node : m_nodes)
                node->render(frame, sqDistanceLimit);
        }

        static base::res::StaticResource<base::image::Image> resBricks("engine/ui/styles/icons/bricks.png");

        void ContentPrefabRoot::buildViewContent(ui::ElementPtr& content)
        {
            auto image = base::RefNew<ui::StaticContent>();
            image->customImage(resBricks.loadAndGet());
            image->name("NodeIcon");

            auto caption = base::RefNew<ui::StaticContent>();
            caption->text(name());
            caption->name("NodeName");
            caption->customMargins(ui::Offsets(5, 0, 0, 0));

            auto leftSide = ui::LayoutHorizontally({ image, caption });

            auto klass = base::RefNew<ui::StaticContent>();
            klass->text("Prefab");
            klass->name("NodeClass");
            klass->styleClasses("italic");
            klass->customMargins(ui::Offsets(5, 0, 0, 0));

            content = ui::LayoutHorizontally({ leftSide, klass });
            content->layoutMode(ui::LayoutMode::Columns);
        }

        /*ContentNodePtr ContentPrefabRoot::nodeContainer_attachNodeTemplate(const scene::NodeTemplatePtr& node)
        {
            // nothing to add
            if (!node)
                return nullptr;

            // was parent specified ?
            scene::NodeLocalContainerID parentLocalNodeID = INDEX_NONE;
            auto parentNode = scene()->objectMap().findObjectOfClass<ContentNode>(parentID);
            if (parentNode)
            {
                // parent node must be part of the same hierarchy
                if (parentNode->nodeContainer() == this)
                {
                    parentLocalNodeID = parentNode->nodeTemplate()->localID();
                }
            }

            // attach node
            auto content = m_prefab->nodeContainer();
            auto createdNodeID = content->attachNode(node, parentLocalNodeID);
            if (INDEX_NONE == createdNodeID)
                return nullptr;

            // find the node template object that was created, make sure they are linked properly
            if (content->findNode(createdNodeID) != node)
                return nullptr;

            // return the content node created for given template node
            return nodeContainer_findContentNode(node);
            return nullptr;
        }*/

        void ContentPrefabRoot::removeNode(const scene::NodeTemplate* node)
        {
            /*auto numNodes = nodes.size();
            for (uint32_t i=0; i<numNodes; ++i)
            {
                auto contentNode = nodes[i];

                if (contentNode->removeNode(node))
                {
                    DEBUG_CHECK(contentNode->documentId() == node->assignedDocumentObjectID());
                    auto nodeId = contentNode->documentId();

                    nodes.erase(i);
                    children.remove(nodeId);

                    contentNode->unlink();
                    break;
                }
            }*/
        }

        /*bool ContentPrefabRoot::nodeContainer_detachNodeTemplate(const scene::NodeTemplatePtr& node)
        {
            // remove node wrapper
            if (auto content = m_prefab->nodeContainer())
            {
                auto localNodeID = node->localID();
                content->detachNode(localNodeID);

                return true;
            }

            // not deleted
            return false;
        }

        base::AbsoluteTransform ContentPrefabRoot::nodeContainer_getNodeAbsoluteTransform(const scene::NodeTemplatePtr& node) const
        {
            return node->placement().toAbsoluteTransform();
        }

        bool ContentPrefabRoot::nodeContainer_setNodeAbsoluteTransform(const scene::NodeTemplatePtr& node, const base::AbsoluteTransform& newTransform) const
        {
            node->placement(newTransform);
            return true;
        }*/

        //---

        void ContentPrefabRoot::syncNodes()
        {
            /*for (auto& node : m_container->rootNodes())
            {
                onRootNodeAttached(m_container.get(), node.get());
            }*/
        }

        /*void ContentPrefabRoot::onRootNodeAttached(const scene::NodeTemplateContainer* container, const scene::NodeTemplate* node)
        {
            // node must have assigned document ID so we can track it in the editor
            auto nodeDocumentID = node->assignedDocumentObjectID();
            DEBUG_CHECK(!nodeDocumentID.empty());

            // create wrapper
            if (!nodeDocumentID.empty())
            {
                if (auto nodeContent = ContentNode::CreateNodeWrapper(scene(), this, node->sharedFromThisType<scene::NodeTemplate>(), ContentNodeSource::Layer, nodeDocumentID))
                {
                    // add to list of children
                    nodes.pushBack(nodeContent);
                    children.add(nodeDocumentID); // updated the tree view
                }
            }
        }

        void ContentPrefabRoot::onRootNodeDetached(const scene::NodeTemplateContainer* container, const scene::NodeTemplate* node)
        {
            removeNode(node);
        }*/

        //---

    } // world
} // ed