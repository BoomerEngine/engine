/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureGroup.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructureWorldRoot.h"
#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneLayer.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(ContentLayer);
        RTTI_END_TYPE();

        //---

        ContentLayer::ContentLayer(const base::StringBuf& layerPath)
            : IContentElement(ContentElementType::Layer)
            , m_filePath(layerPath)
            , m_name(layerPath.view().afterLast("/").beforeFirst("."))
            , m_modified(false)
        {
        }

        ContentLayer::~ContentLayer()
        {
        }

        void ContentLayer::unmarkModified()
        {
            if (m_modified)
            {
                requestViewUpdate(ContentDirtyFlagBit::Modified);
                requestViewUpdate(ContentDirtyFlagBit::Name); // to add the "*"
                m_modified = false;
            }
        }

        void ContentLayer::markModified()
        {
            TBaseClass::markModified();

            if (!m_modified)
            {
                requestViewUpdate(ContentDirtyFlagBit::Modified);
                requestViewUpdate(ContentDirtyFlagBit::Name); // to add the "*"
                m_modified = true;
            }
        }

        const base::StringBuf& ContentLayer::name() const
        {
            return m_name;
        }

        void ContentLayer::syncNodes(const depot::ManagedFilePtr& layerFile)
        {
            ASSERT(!layerFile->isDeleted());

            if (auto layer = layerFile->loadEditableContent<scene::Layer>())
            {
                if (auto content = layer->nodeContainer())
                {
                    for (auto& rootNodeId : content->rootNodes())
                    {
                        auto rootNodeTemplate = content->nodes()[rootNodeId].m_data;

                        if (auto nodeTemplateCopy = base::CloneObject<scene::NodeTemplate>(rootNodeTemplate))
                        {
                            if (auto nodeContent = base::CreateSharedPtr<ContentNode>(nodeTemplateCopy))
                            {
                                m_nodes.pushBack(nodeContent);
                                nodeContent->attachToParent(this);
                            }
                        }
                    }

                    TRACE_INFO("Loaded {} nodes from layer '{}'", content->rootNodes().size(), m_filePath);
                    unmarkModified();
                }
            }
            else 
            {
                TRACE_WARNING("Unable to sync layer '{}' nodes from file because we can't load it", m_filePath);
            }
        }

        bool ContentLayer::saveIntoNodeContainer(const scene::NodeTemplateContainerPtr& nodeContainer)
        {
            for (auto& node : m_nodes)
            {
                if (auto nodeTemplate = node->nodeTemplate())
                {
                    auto nodeId = nodeContainer->addNode(nodeTemplate);

                        // TODO: attach children
                }
            }

            TRACE_INFO("Saved {} root nodes ({} total) into content for layer '{}'", nodeContainer->rootNodes().size(), nodeContainer->nodes().size(), m_filePath);
            return true;
        }

        /*bool ContentLayer::save(const ui::ElementPtr& owner, bool notify)
        {
            auto ret = m_fileHandler->saveEditableContent();

            if (notify)
            {
                if (ret)
                    ui::PostGeneralMessageWindow(owner, ui::MessageType::Info, base::TempString("Layer '{}' saved", name()));
                else
                    ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, base::TempString("Failed to save layer '{}'", name()));
            }

            refreshModificationState();
            return true;
        }*/

        ui::ModelIndex ContentLayer::modelIndex() const
        {
            if (auto group  = parent())
            {
                for (uint32_t i = 0; i < group->layers().size(); ++i)
                {
                    if (group->layers()[i].get() == this)
                    {
                        return ui::ModelIndex(scene(), i + group->groups().size(), 0, sharedFromThis().toWeak());
                    }
                }
            }

            return ui::ModelIndex();
        }

        base::ObjectPtr ContentLayer::resolveObject(base::ClassType expectedClass) const
        {
            return nullptr;
        }

        void ContentLayer::visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const
        {
            if (!func(sharedFromThis()))
                return;

            for (auto& node : m_nodes)
                node->visitStructure(func);
        }

        void ContentLayer::collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const
        {
            if (isModified())
                outModifiedContent.pushBack(sharedFromThis());
        }

        void ContentLayer::render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const
        {
            if (!mergedVisibility())
                return;

            for (auto& node : m_nodes)
                node->render(frame, sqDistanceLimit);
        }

        static base::res::StaticResource<base::image::Image> resLayer("engine/ui/styles/icons/table.png");
        static base::res::StaticResource<base::image::Image> resLayerEdit("engine/ui/styles/icons/table_edit.png");

        base::image::ImagePtr ContentLayer::layerIcon() const
        {
            auto isActive = m_structure && m_structure->activeElement().get() == this;
            return isActive ? resLayerEdit.loadAndGet() : resLayer.loadAndGet();
        }

        base::StringBuf ContentLayer::captionText() const
        {
            auto isActive = m_structure && m_structure->activeElement().get() == this;
            if (isActive)
                return base::TempString("{} [{}] (Active)", name(), m_nodes.size());
            else
                return base::TempString("{} [{}]", name(), m_nodes.size());
        }

        base::StringBuf ContentLayer::captionStyle() const
        {
            auto isActive = m_structure && m_structure->activeElement().get() == this;
            if (isActive)
                return "bold";
            else
                return "transparent;italic";
        }

        void ContentLayer::buildViewContent(ui::ElementPtr& content)
        {
            auto image = base::CreateSharedPtr<ui::StaticContent>();
            image->customImage(layerIcon());
            image->name("NodeIcon");

            auto caption = base::CreateSharedPtr<ui::StaticContent>();
            caption->text(captionText());
            caption->name("NodeName");
            caption->styleClasses(captionStyle().c_str());
            caption->customMargins(ui::Offsets(5, 0, 0, 0));

            auto leftSide = ui::LayoutHorizontally({ image, caption });

            auto klass = base::CreateSharedPtr<ui::StaticContent>();
            klass->text("Layer");
            klass->name("NodeClass");
            klass->styleClasses("italic");
            klass->customMargins(ui::Offsets(5, 0, 0, 0));

            content = ui::LayoutHorizontally({ leftSide, klass });
            content->layoutMode(ui::LayoutMode::Columns);
        }

        void ContentLayer::refreshViewContent(ui::ElementPtr& content)
        {
            if (m_dirtyFlags.test(ContentDirtyFlagBit::Name))
            {
                if (auto caption = content->findChildByName<ui::StaticContent>("NodeName"))
                {
                    caption->text(captionText());
                    caption->styleClasses(captionStyle().c_str());
                }

                if (auto image = content->findChildByName<ui::StaticContent>("NodeIcon"))
                {
                    image->customImage(layerIcon());
                }

                m_dirtyFlags -= ContentDirtyFlagBit::Name;
            }
        }

        /*void ContentLayer::refreshUIContent(uint32_t column, ui::IElement* owner, ui::ElementPtr& content) const
        {
            if (column == COL_NAME)
            {
                if (content)
                {
                    if (m_nodeCaptionChanged)
                    {
                        if (auto name = content->findChildByName<ui::StaticContent>("LayerName"))
                        {
                            name->text(captionText());
                            name->styleClasses(captionStyle().c_str());
                        }

                        if (auto name = content->findChildByName<ui::StaticContent>("LayerIcon"))
                        {
                            name->customImage(layerIcon());
                        }

                        m_nodeCaptionChanged = false;
                    }
                }
                else
                {
                    auto image = base::CreateSharedPtr<ui::StaticContent>();
                    image->customImage(layerIcon());
                    image->name("LayerIcon");

                    auto caption = base::CreateSharedPtr<ui::StaticContent>();
                    caption->text(captionText());
                    caption->name("LayerName");
                    caption->customMargins(ui::Offsets(5,0,0,0));

                    content = ui::LayoutHorizontally({image, caption});
                    owner->attachChild(content);
                }
            }

            TBaseClass::refreshUIContent(column, owner, content);
        }*/

        void ContentLayer::refreshVisualizationVisibility()
        {
            for (auto& node : m_nodes)
                node->refreshMergedVisibility();
        }

        int ContentLayer::compareType() const
        {
            return 2;
        }

        bool ContentLayer::compareForSorting(const IContentElement& other) const
        {
            auto& otherLayer = static_cast<const ContentLayer&>(other);
            return name() < otherLayer.name();
        }

        ContentElementMask ContentLayer::contentType() const
        {
            auto ret = TBaseClass::contentType();

            ret |= ContentElementBit::CanToggleVisibility;
            ret |= ContentElementBit::NodeContainer;
            ret |= ContentElementBit::CanBePasteRoot;
            ret |= ContentElementBit::Layer;
            ret |= ContentElementBit::CanActivate;
            ret |= ContentElementBit::CanSave;

            if (m_modified)
                ret |= ContentElementBit::Modified;

            return ret;
        }

        bool ContentLayer::addContent(const base::Array<ContentElementPtr>& content)
        {
            if (content.empty())
                return true;

            for (auto& ptr : content)
            {
                auto node = base::rtti_cast<ContentNode>(ptr);
                if (!node)
                    return false;
            }

            base::HashSet<base::StringID> usedNames;

            for (auto& node : m_nodes)
                usedNames.insert(node->nodeTemplate()->name());

            for (auto& ptr : content)
            {
                auto node = base::rtti_cast<ContentNode>(ptr);

                // assight unique name
                node->generateUniqueNodeName(usedNames);
                usedNames.insert(node->nodeTemplate()->name());

                // add to list
                m_nodes.pushBack(node);
                node->attachToParent(this);
            }

            markModified();
            return true;
        }

        bool ContentLayer::removeContent(const base::Array<ContentElementPtr>& content)
        {
            if (content.empty())
                return true;

            for (auto& ptr : content)
            {
                auto node = base::rtti_cast<ContentNode>(ptr);
                if (!node)
                    return false;

                if (node->parent() != this)
                    return false;

                if (!m_nodes.contains(node))
                    return false;
            }

            for (auto& ptr : content)
            {
                auto node = base::rtti_cast<ContentNode>(ptr);
                node->detachFromParent();
                m_nodes.remove(node);
            }

            markModified();
            return true;
        }

        //--

        /*ContentNodePtr ContentLayer::nodeContainer_attachNodeTemplate(const scene::NodeTemplatePtr& node)
        {
            // no layer
            if (!m_loadedLayer)
                return nullptr;

            // nothing to add
            if (!node)
                return nullptr;

            // attach node to the content
            auto createdNodeID = m_loadedLayer->nodeContainer()->attachNode(node);
            if (INDEX_NONE == createdNodeID)
                return nullptr;

            // allocate the document ID for this node
            auto newNodeDocumentID = base::edit::DocumentObjectID::CreateNewObjectID("SceneContentNode"_id);

            // create wrapper
            if (auto nodeContent = ContentNode::CreateNodeWrapper(scene(), this, node, ContentNodeSource::Layer, newNodeDocumentID))
            {
                nodes.pushBack(nodeContent);
                return nodeContent;
            }

            return nullptr;
        }*/

        void ContentLayer::removeNode(const scene::NodeTemplate* node)
        {
            /*// remove node wrapper
            auto numNodes = nodes.size();
            for (uint32_t i=0; i<numNodes; ++i)
            {
                auto contentNode = nodes[i];
                if (contentNode->removeNode(node))
                {
                    nodes.erase(i);

                    //children.remove(nodeId);
                    m_nodeCaptionChanged = true;

                    contentNode->unlink();
                    break;
                }
            }*/
        }

        /*bool ContentLayer::nodeContainer_detachNodeTemplate(const scene::NodeTemplatePtr& node)
        {
            // get the node storage
            if (auto content = nodeContainer_getNodeTemplateContainer())
            {
                auto localNodeID = node->localID();
                content->detachNode(localNodeID);
                return true;
            }

            // not removed
            return false;
        }

        base::AbsoluteTransform ContentLayer::nodeContainer_getNodeAbsoluteTransform(const scene::NodeTemplatePtr& node) const
        {
            return node->placement().toAbsoluteTransform();
        }

        bool ContentLayer::nodeContainer_setNodeAbsoluteTransform(const scene::NodeTemplatePtr& node, const base::AbsoluteTransform& newTransform) const
        {
            node->placement(newTransform);
            return true;
        }*/

/*        void ContentLayer::onRootNodeAttached(const scene::NodeTemplateContainer* container, const scene::NodeTemplate* node)
        {
            // node must have assigned document ID so we can track it in the editor
            auto nodeDocumentID = node->assignedDocumentObjectID();
            if (nodeDocumentID.empty())
                return;

            // create wrapper
            if (auto nodeContent = ContentNode::CreateNodeWrapper(scene(), this, node->sharedFromThisType<scene::NodeTemplate>(), ContentNodeSource::Layer, nodeDocumentID))
            {
                // add to list of children
                nodes.pushBack(nodeContent);
                children.add(nodeDocumentID); // updated the tree view
            }
        }

        void ContentLayer::onRootNodeDetached(const scene::NodeTemplateContainer* container, const scene::NodeTemplate* node)
        {
            removeNode(node);
        }*/

    } // world
} // ed