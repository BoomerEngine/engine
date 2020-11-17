/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"

#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneLayer.h"
#include "scene/common/include/scenePrefab.h"
#include "scene/common/include/sceneNodeTemplate.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(ContentNode);
        RTTI_END_TYPE();

        //---

        ContentNode::ContentNode(const scene::NodeTemplatePtr& nodeTemplate)
            : IContentElement(ContentElementType::Node)
            , m_nodeTemplate(nodeTemplate)
            , m_visualizationSelectionFlag(false)
            , m_localVisualizationSelectionFlag(false)
        {
            m_absoluteTransform = nodeTemplate->placement().toAbsoluteTransform();
            syncNodes();

            /*m_nodeTemplateObserver.bindCallback([this](OBJECT_EVENT_FUNC)
                {
                    if (eventID == "OnPropertyChanged"_id)
                    {
                        propertyChanged(eventPath);
                    }
                });

            m_nodeTemplateObserver.bindObject(m_nodeTemplate->id());*/
        }

        ContentNode::~ContentNode()
        {
        }

        const base::StringBuf& ContentNode::name() const
        {
            return base::StringBuf::EMPTY();// m_nodeTemplate->name().view();
        }

        void ContentNode::propertyChanged(base::StringView path)
        {
            if (path.beginsWith("name"))
            {
                requestViewUpdate(ContentDirtyFlagBit::Name);
            }
            else if (path.beginsWith("placement"))
            {
                syncTransfromFromNode();
            }
            else if (m_nodeTemplate)
            {
                syncPropertiesFromNode();
            }

            markModified();
        }

        void ContentNode::syncPropertiesFromNode()
        {
            /*if (m_visualizationProxy)
            {
                if (auto node = m_visualizationProxy->node())
                {
                    if (!node->handleUpdateTemplatePreview(*m_nodeTemplate))
                    {
                        refreshVisualization();
                    }
                }
            }*/
        }

        void ContentNode::syncNodes()
        {
            //auto nodeID = m_nodeTemplate->localID();
            /*for (auto &node : nodeContainer()->nodeContainer_getNodeTemplateContainer()->nodes())
            {
                if (node->parentID() == nodeID)
                {
                    if (node->assignedDocumentObjectID().empty())
                    {
                        auto newNodeDocumentID = base::edit::DocumentObjectID::CreateNewObjectID("SceneContentNode"_id);
                        node->documentObjectID(newNodeDocumentID);
                    }

                    onNodeChildAdded(m_nodeTemplate.get(), node.get());
                }
            }*/
        }

        void ContentNode::markModified()
        {
            TBaseClass::markModified();

            if (parent())
                parent()->markModified();
        }

        void ContentNode::attachToScene(const scene::ScenePtr& scene)
        {
            TBaseClass::attachToScene(scene);
            refreshVisualization();
        }

        void ContentNode::detachFromScene(const scene::ScenePtr& scene)
        {
            destroyVisualization();
            TBaseClass::detachFromScene(scene);
        }

        void ContentNode::attachToStructure(ContentStructure* structure)
        {
            TBaseClass::attachToStructure(structure);

            if (m_localVisualizationSelectionFlag)
                scene()->updateVisualSelectionList(*this, true);
        }

        void ContentNode::detachFromStructure()
        {
            if (m_localVisualizationSelectionFlag)
            {
                scene()->updateVisualSelectionList(*this, false);
                m_localVisualizationSelectionFlag = false;
            }

            TBaseClass::detachFromStructure();
        }

        void ContentNode::refreshVisualizationSelectionFlag()
        {
            auto flag = calcVisualizationSelectionFlag();
            if (flag != m_visualizationSelectionFlag)
            {
                m_visualizationSelectionFlag = flag;

                /*if (m_visualizationProxy)
                    m_visualizationProxy->selectionFlag(flag);*/
            }

            for (auto& node : m_childNodes)
                node->refreshVisualizationSelectionFlag();
        }

        bool ContentNode::calcVisualizationSelectionFlag() const
        {
            /*// prefab instanced nodes
            if (isInstancedFromPrefab())
            {
                // if prefab root is selected select everything anyway
                if (auto prefabNode = findPrefabOwnerNode())
                    if (prefabNode->m_localVisualizationSelectionFlag)
                        return true;
            }*/

            // just use local flag
            return m_localVisualizationSelectionFlag;
        }

        void ContentNode::destroyVisualization()
        {
            /*if (m_visualizationProxy)
            {
                scene()->streamingGrid()->unregisterNode(m_visualizationProxy);
                m_visualizationProxy = nullptr;
            }*/
        }

        void ContentNode::refreshVisualization()
        {
            /*if (auto streamingGrid  = scene()->streamingGrid())
            {
                // cancel any pending streaming
                if (m_visualizationProxy)
                {
                    streamingGrid->unregisterNode(m_visualizationProxy);
                    m_visualizationProxy = nullptr;
                }

                // create new visualization only if we are allowed to be visible
                if (mergedVisibility())
                {
                    // create the node visualization
                    scene::NodePreviewInstanceSetupInfo context;
                    context.m_isSelected = calcVisualizationSelectionFlag();
                    context.m_selectableId = id();
                    context.m_template = m_nodeTemplate;
                    context.m_absoluteTransform = m_absoluteTransform;

                    // set custom streaming distance
                    if (m_nodeTemplate->visibilityDistanceOverride() > 0)
                        context.m_customStreamingDistance = m_nodeTemplate->visibilityDistanceOverride();

                    // create proxy
                    ASSERT(m_visualizationProxy == nullptr);
                    m_visualizationProxy = streamingGrid->registerProxy(context);
                }
            }*/
        }

        void ContentNode::visualSelectionFlag(bool flag, bool updateStructureList)
        {
            if (flag != m_localVisualizationSelectionFlag)
            {
                m_localVisualizationSelectionFlag = flag;

                if (updateStructureList)
                {
                    if (auto scene  = this->scene())
                        scene->updateVisualSelectionList(*this, flag);
                }

                refreshVisualizationSelectionFlag();
            }
        }

        void ContentNode::refreshViewContent(ui::ElementPtr& content)
        {
            TBaseClass::refreshViewContent(content);
        }

        static base::res::StaticResource<base::image::Image> resNode("engine/ui/styles/icons/viewmode.png");

        void ContentNode::buildViewContent(ui::ElementPtr& content)
        {
            auto image = base::CreateSharedPtr<ui::StaticContent>();
            image->customImage(resNode.loadAndGet());
            image->name("NodeIcon");

            auto caption = base::CreateSharedPtr<ui::StaticContent>();
            caption->text(captionText());
            caption->name("NodeName");
            caption->styleClasses(captionStyle().c_str());
            caption->customMargins(ui::Offsets(5, 0, 0, 0));

            auto leftSide = ui::LayoutHorizontally({ image, caption });

            auto node = static_cast<ContentNode*>(this);
            auto className = node->nodeTemplate()->cls()->name();

            auto klass = base::CreateSharedPtr<ui::StaticContent>();
            //klass->text(className.buffer().stringAfterLast("::"));
            klass->name("NodeClass");
            klass->styleClasses("italic");
            klass->customMargins(ui::Offsets(5, 0, 0, 0));

            content = ui::LayoutHorizontally({ leftSide, klass });
            content->layoutMode(ui::LayoutMode::Columns);
        }

        void ContentNode::refreshVisualizationVisibility()
        {
            // destroy/recreate proxy
            /*if (mergedVisibility() && !m_visualizationProxy)
                refreshVisualization();
            else if (!mergedVisibility() && m_visualizationProxy)
                refreshVisualization();*/

            // update child nodes
            for (auto& child : m_childNodes)
                child->refreshMergedVisibility();
        }

        void ContentNode::syncTransfromFromNode()
        {
            auto newTransform = m_nodeTemplate->placement().toAbsoluteTransform();
            if (newTransform != m_absoluteTransform)
            {
                m_absoluteTransform = newTransform;
                applyTransform();
            }
        }

        void ContentNode::applyTransform()
        {
            /*if (m_visualizationProxy)
            {
                // update visualization stuff
                m_visualizationProxy->absoluteTransform(m_absoluteTransform);

                // move in grid
                scene()->streamingGrid()->updateNodePlacement(m_visualizationProxy, m_absoluteTransform.position());
            }*/
        }

        ContentElementMask ContentNode::contentType() const
        {
            auto ret = TBaseClass::contentType();

            ret |= ContentElementBit::Node;
            ret |= ContentElementBit::HasObject;
            ret |= ContentElementBit::CanCopy;
            ret |= ContentElementBit::CanToggleVisibility;

            if (isInstancedFromPrefab())
            {
                ret |= ContentElementBit::FromPrefab;
            }
            else
            {
                ret |= ContentElementBit::CanDelete;
                ret |= ContentElementBit::CanBePasteRoot;
                ret |= ContentElementBit::NodeContainer;
                ret |= ContentElementBit::CanActivate;
            }

            return ret;
        }
        
        base::Box ContentNode::calcWorldSpaceBoundingBox() const
        {
            /*// use the visualization proxy if we have one
            if (m_visualizationProxy != nullptr)
            {
                auto node = m_visualizationProxy->node();
                if (node)
                    return node->calcWorldSpaceBoundingBox();
            }

            // TODO: use the estimation from the node template          
            auto localToWorld = m_absoluteTransform.approximate();
            return m_nodeTemplate->calcWorldSpaceBoundingBoxEstimate(localToWorld);*/

            auto localToWorld = m_absoluteTransform.approximate();
            return base::Box(localToWorld.translation(), 2.0f);
        }

        void ContentNode::absoluteTransform(const base::AbsoluteTransform& transform)
        {
            m_absoluteTransform = transform;
            m_nodeTemplate->placement(transform);
            applyTransform();
        }

        static base::StringID GetNodeIcon(const ContentNode& node)
        {
            return "bullet_yellow"_id;
        }

        static base::StringBuf GetNodeClassName(const ContentNode& node)
        {
            /*auto nodeClass  = node.nodeTemplate()->cls();

            if (auto nameMetadata = nodeClass->findMetadata<scene::NodeTemplateClassName>())
                return base::TempString("({})", nameMetadata->name());

            auto shortClassName = nodeClass->name().view().afterLast("::");
            return base::TempString("({})", shortClassName);*/
            return base::StringBuf("Node");
        }

        base::StringBuf ContentNode::captionText() const
        {
            auto isActive = m_structure && m_structure->activeElement().get() == this;
            if (isActive)
                return base::TempString("{} (Active)", m_nodeTemplate->name());
            else
                return base::TempString("{}",m_nodeTemplate->name());
        }

        base::StringBuf ContentNode::captionStyle() const
        {
            auto isActive = m_structure && m_structure->activeElement().get() == this;
            if (isInstancedFromPrefab())
                return "prefabInstance;italic";
            else if (isActive)
                return "bold";
            else
                return "";
        }
        
        /*void ContentNode::refreshUIContent(uint32_t column, ui::IElement* owner, ui::ElementPtr& content) const
        {
            if (column == COL_NAME)
            {
                if (content)
                {
                    if (m_captionChanged)
                    {
                        if (auto name = content->findChildByName<ui::StaticContent>("NodeName"))
                        {
                            name->text(captionText());
                            name->styleClasses(captionStyle().c_str());
                        }

                        m_captionChanged = false;
                    }
                }
                else
                {
                    auto image = base::CreateSharedPtr<ui::StaticContent>();
                    image->customImage(GetNodeIcon(*this));
                    image->name("NodeIcon");

                    auto caption = base::CreateSharedPtr<ui::StaticContent>();
                    caption->text(captionText());
                    caption->name("NodeName");
                    caption->styleClasses(captionStyle().c_str());
                    caption->customMargins(ui::Offsets(5,0,0,0));

                    auto klass = base::CreateSharedPtr<ui::StaticContent>();
                    klass->text(GetNodeClassName(*this));
                    klass->name("NodeClass");
                    klass->styleClasses("italic");
                    klass->customMargins(ui::Offsets(5,0,0,0));

                    content = ui::LayoutHorizontally({image, caption, klass});

                    owner->attachChild(content);
                }
            }

            // create base icons
            TBaseClass::refreshUIContent(column, owner, content);
        }*/

        /*void ContentNode::absoluteTransform(const base::AbsoluteTransform& transform)
        {
            auto nodeContainer = parent()->nodeContainer();
            if (nodeContainer)
            {
                m_nodeTemplate->placement(transform);
            }
        }*/

        void ContentNode::extractUsedResource(base::HashSet<base::res::ResourcePath>& outResourcePaths) const
        {
            //m_nodeTemplate->extractUsedResource(outResourcePaths);
        }

        int ContentNode::compareType() const
        {
            return 3;
        }

        bool ContentNode::compareForSorting(const IContentElement& other) const
        {
            //auto& otherNode = static_cast<const ContentNode&>(other);
            //return m_nodeTemplate->name().buffer().compareWithNoCase(otherNode.m_nodeTemplate->name().buffer()) < 0;
            return false;
        }

        void ContentNode::render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const
        {
            // not visible
            if (!mergedVisibility())
                return;

            // render the node template content
            if (m_nodeTemplate)
            {
                auto squareDist = m_absoluteTransform.position().approximate().squareDistance(frame.camera().position());
                if (squareDist < sqDistanceLimit)
                {
                    //m_nodeTemplate->renderEditorFragments(frame, m_absoluteTransform, id(), m_visualizationSelectionFlag);
                }
            }

            // recurse
            TBaseClass::render(frame, sqDistanceLimit);
        }

        void ContentNode::collectHierarchy(base::HashSet<ContentNode*>& nodes)
        {
            if (!nodes.insert(this))
            {
                for (auto& child : m_childNodes)
                    child->collectHierarchy(nodes);
            }
        }

        bool ContentNode::isInstancedFromPrefab() const
        {
            //auto parent  = parent();
            //return parent && parent->is<ContentPrefabNode>();
            return false;
        }
        
        bool ContentNode::removeNode(const scene::NodeTemplate* node)
        {
            if (nodeTemplate().get() == node)
                return true;

            auto numChildren = m_childNodes.size();
            for (uint32_t i=0; i<numChildren; ++i)
            {
                auto childNode = m_childNodes[i];
                if (childNode->removeNode(node))
                {
                    childNode->detachAllChildren();
                    childNode->detachFromParent();
                    m_childNodes.erase(i);
                    break;
                }
            }

            return false;
        }

        void ContentNode::generateUniqueNodeName(const base::HashSet<base::StringID>& existingNames)
        {
            auto baseName = m_nodeTemplate->name();
            if (baseName && !existingNames.contains(baseName))
                return;

            if (!baseName)
            {
                auto className = m_nodeTemplate->cls()->name().view().afterLastOrFull("::");
                baseName = className;
            }
            else
            {
                baseName = baseName.view().trimTailNumbers();
            }

            auto testName = base::StringID(baseName);

            uint32_t index = 1;
            for (;;)
            {
                if (!existingNames.contains(testName))
                {
                    m_nodeTemplate->name(testName);
                    break;
                }

                testName = base::StringID(base::TempString("{}{}", baseName, index));
                index += 1;
            }
        }


        /*void ContentNode::onNodeTransformChanged(const scene::NodeTemplate* nodeTemplate)
        {
            applyTransform();
        }

        void ContentNode::onNodePropertyChanged(const scene::NodeTemplate* nodeTemplate, const base::view::AccessPath& path)
        {
            if (path.beginsWithPropertyName("name"_id))
            {
                m_captionChanged = true;
                scene()->requestItemUpdate(modelIndex());
            }
            else
            {
                refreshVisualization();
            }
        }

        void ContentNode::onNodeChildAdded(const scene::NodeTemplate* nodeTemplate, const scene::NodeTemplate* childNodeTemplate)
        {
            // node must have assigned document ID so we can track it in the editor
            auto nodeDocumentID = childNodeTemplate->assignedDocumentObjectID();
            if (nodeDocumentID.empty())
                return;

            // check if already have the node
            for (auto& child : m_childNodes)
                if (child->nodeTemplate().get() == childNodeTemplate)
                    return;

            // create node wrapper
            if (auto nodeContent = ContentNode::CreateNodeWrapper(scene(), this, childNodeTemplate->sharedFromThisType<scene::NodeTemplate>(), sourceType(), nodeDocumentID))
            {
                m_childNodes.pushBack(nodeContent);
                children.add(nodeDocumentID);
            }
        }

        void ContentNode::onNodeChildRemoved(const scene::NodeTemplate* nodeTemplate, const scene::NodeTemplate* childNodeTemplate)
        {
            // check if already have the node
            for (auto& child : m_childNodes)
            {
                if (child->nodeTemplate().get() == childNodeTemplate)
                {
                    children.remove(child->documentId());
                    child->unlink();
                    m_childNodes.remove(child);
                    break;
                }
            }
        }*/

        //---

        scene::NodePath ContentNode::path() const
        {
            auto nodeName = nodeTemplate()->name();
            return parent()->path()[nodeName];
        }

        base::ObjectPtr ContentNode::resolveObject(base::ClassType expectedClass) const
        {
            return m_nodeTemplate;
        }

        void ContentNode::visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const
        {
            if (!func(sharedFromThis()))
                return;

            for (auto& comp : m_childNodes)
                comp->visitStructure(func);
        }
        
        //--

    } // world
} // ed