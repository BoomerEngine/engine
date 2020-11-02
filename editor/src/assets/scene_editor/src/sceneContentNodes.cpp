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

#include "game/world/include/worldNodeContainer.h"
#include "game/world/include/worldNodeTemplate.h"
#include "game/world/include/worldNodePlacement.h"
#include "game/world/include/worldEntityTemplate.h"
#include "game/world/include/worldComponentTemplate.h"
#include "game/world/include/worldPrefab.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "base/editor/include/managedFileNativeResource.h"
#include "base/object/include/objectTemplate.h"

namespace ed
{
    
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentNode);
    RTTI_END_TYPE();

    SceneContentNode::SceneContentNode(SceneContentNodeType type, const StringBuf& name)
        : m_type(type)
        , m_name(name)
    {}

    void SceneContentNode::name(const StringBuf& name)
    {
        if (m_name != name)
        {
            m_name = name;

            if (auto parentNode = parent())
                parentNode->m_childrenNames.clear();

            SceneContentNodePtr selfPtr(AddRef(this));

            postEvent(EVENT_CONTENT_NODE_RENAMED, selfPtr);

            if (m_structure)
                m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_RENAMED, selfPtr);
        }
    }

    StringBuf SceneContentNode::buildUniqueName(const StringBuf& name, bool userGiven, const HashSet<StringBuf>* additionalTakenName) const
    {
        if (m_childrenNames.empty() && !m_children.empty())
        {
            m_childrenNames.reserve(m_children.size());
            for (const auto& child : m_children)
            {
                DEBUG_CHECK(!child->name().empty());
                if (!child->name().empty())
                {
                    auto added = m_childrenNames.insert(child->name());
                    DEBUG_CHECK_EX(added, "Duplicated child name");
                }
            }
        }

        if (userGiven && !name.empty())
            if (!m_childrenNames.contains(name))
                if (!additionalTakenName || !additionalTakenName->contains(name))
                    return name;

        StringBuf coreName = name;
        if (coreName.empty())
            coreName = "node";

        uint32_t counter = 0;
        coreName = StringBuf(coreName.view().trimTailNumbers(&counter));

        StringBuilder txt;
        do
        {
            txt.clear();
            txt << coreName;
            txt << counter;
            counter += 1;
        } while (m_childrenNames.contains(txt.view()) || (additionalTakenName && additionalTakenName->contains(txt.view())));
    
        return txt.toString();
    }

    void SceneContentNode::displayText(IFormatStream& txt) const
    {
        if (m_visualFlags.test(SceneContentNodeVisualBit::ActiveNode))
            txt << "[b][i]";

        switch (m_type)
        {
            case SceneContentNodeType::Entity: txt << "[img:node]"; break;
            case SceneContentNodeType::LayerFile: txt << "[img:page]"; break;
            case SceneContentNodeType::LayerDir: txt << "[img:folder]"; break;
            case SceneContentNodeType::PrefabRoot: txt << "[img:brick]"; break;
            case SceneContentNodeType::WorldRoot: txt << "[img:world]"; break;
        }

        txt << " ";
        txt << m_name;

        if (m_visualFlags.test(SceneContentNodeVisualBit::ActiveNode))
            txt << "[/i][/b]";

        if (m_visualFlags.test(SceneContentNodeVisualBit::ActiveNode))
            txt << " [tag:#8A8]Active[/tag]";
    }

    bool SceneContentNode::contains(SceneContentNode* node) const
    {
        const auto* ptr = this;
        while (ptr != nullptr)
        {
            if (ptr == node)
                return true;

            ptr = ptr->parent();
        }

        return false;
    }

    void SceneContentNode::bindRootStructure(SceneContentStructure* structure)
    {
        DEBUG_CHECK_RETURN(parent() == nullptr);
        
        if (m_structure)
            conditionalDetachFromStructure();

        m_structure = structure;

        if (m_structure)
        {
            m_structure->handleNodeAdded(this);
            handleAttachedToStructure();

            for (const auto& child : m_children)
                child->conditionalAttachToStructure();
        }
    }

    void SceneContentNode::attachChildNode(SceneContentNode* child)
    {
        DEBUG_CHECK_RETURN(child);
        DEBUG_CHECK_RETURN(child->parent() == nullptr);
        DEBUG_CHECK_RETURN(child != this); // common mistake
        DEBUG_CHECK_RETURN(!child->contains(this));
        DEBUG_CHECK_RETURN(!contains(child));

        child->IObject::parent(this);
        child->handleParentChanged();

        m_children.pushBack(AddRef(child));

        if (auto entity = rtti_cast<SceneContentEntityNode>(child))
            m_entities.pushBack(AddRef(entity));
        
        handleChildAdded(child);

        child->conditionalAttachToStructure();

        m_childrenNames.insert(child->name());
    }

    void SceneContentNode::detachAllChildren()
    {
        const auto children = m_children;

        for (auto i : children.indexRange().reversed())
            detachChildNode(children[i]);

        m_childrenNames.clear();
    }

    void SceneContentNode::detachChildNode(SceneContentNode* child)
    {
        DEBUG_CHECK_RETURN(child);
        DEBUG_CHECK_RETURN(child->parent() == this);

        SceneContentNodePtr childRef(AddRef(child));

        m_children.remove(child);

        if (auto entity = rtti_cast<SceneContentEntityNode>(child))
            m_entities.remove(entity);

        if (child->m_structure)
            child->conditionalDetachFromStructure();

        handleChildRemoved(child);

        child->IObject::parent(nullptr);
        child->handleParentChanged();

        m_childrenNames.clear();
    }

    void SceneContentNode::propagateMergedVisibilityStateFromThis()
    {
        auto parentNode = parent();
        auto parentVisibilityState = parentNode ? parentNode->m_visible : true;
        propagateMergedVisibilityState(parentVisibilityState);
    }

    void SceneContentNode::propagateMergedVisibilityState(bool parentVisibilityState)
    {
        const auto newState = m_localVisibilityFlag && parentVisibilityState;
        if (newState != m_visible)
        {
            m_visible = newState;

            handleVisibilityChanged();

            for (const auto& child : m_children)
                child->propagateMergedVisibilityState(m_visible);
        }
    }

    void SceneContentNode::visibility(bool flag)
    {
        if (flag != m_localVisibilityFlag)
        {
            m_localVisibilityFlag = flag;
            propagateMergedVisibilityStateFromThis();
        }
    }

    void SceneContentNode::visualFlag(SceneContentNodeVisualBit flag, bool value)
    {
        bool changed = false;

        if (value && !m_visualFlags.test(flag))
        {
            m_visualFlags.set(flag);
            changed = true;
        }
        else if (!value && m_visualFlags.test(flag))
        {
            m_visualFlags.clear (flag);
            changed = true;
        }

        if (changed)
        {
            SceneContentNodePtr selfRef(AddRef(this));

            if (m_structure)
                m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED, selfRef);

            postEvent(EVENT_CONTENT_NODE_VISUAL_FLAG_CHANGED, selfRef);
        }
    }

    //--

    void SceneContentNode::handleParentChanged()
    {

    }

    void SceneContentNode::handleChildAdded(SceneContentNode* child)
    {
        DEBUG_CHECK_RETURN(child);
        DEBUG_CHECK_RETURN(child->parent() == this);
        DEBUG_CHECK_RETURN(m_children.contains(child));

        SceneContentNodePtr childRef(AddRef(child));

        postEvent(EVENT_CONTENT_NODE_ADDED, childRef);

        if (m_structure)
            m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_ADDED, childRef);
    }

    void SceneContentNode::handleChildRemoved(SceneContentNode* child)
    {
        DEBUG_CHECK_RETURN(child);
        DEBUG_CHECK_RETURN(child->parent() == this);
        DEBUG_CHECK_RETURN(!m_children.contains(child));

        SceneContentNodePtr childRef(AddRef(child));

        if (m_structure)
        {
            if (auto parentNode = parent())
                handleDetachedFromStructure();

            m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_REMOVED, childRef);
        }

        postEvent(EVENT_CONTENT_NODE_REMOVED, childRef);
    }

    void SceneContentNode::conditionalAttachToStructure()
    {
        DEBUG_CHECK_RETURN(m_structure == nullptr);

        if (auto parentNode = parent())
        {
            if (auto parentStructure = parentNode->m_structure)
            {
                m_structure = parentStructure;
                m_structure->handleNodeAdded(this);

                handleAttachedToStructure();
            }
        }

        if (m_structure != nullptr)
            for (const auto& child : m_children)
                child->conditionalAttachToStructure();
    }

    void SceneContentNode::conditionalDetachFromStructure()
    {
        DEBUG_CHECK_RETURN(m_structure != nullptr);

        if (m_structure != nullptr)
            for (const auto& child : m_children)
                child->conditionalDetachFromStructure();

        handleDetachedFromStructure();

        m_structure->handleNodeRemoved(this);
        m_structure = nullptr;
    }

    void SceneContentNode::conditionalCreateVisualization()
    {
        const auto shouldHave = m_structure && m_visible;
        if (shouldHave && !m_visualizationCreated)
        {
            m_visualizationCreated = true;
            handleCreateVisualization();
        }
    }

    void SceneContentNode::conditionalDestroyVisualization()
    {
        const auto shouldHave = m_structure && m_visible;
        if (!shouldHave && m_visualizationCreated)
        {
            m_visualizationCreated = false;
            handleDestroyVisualization();
        }
    }

    void SceneContentNode::conditionalRecreateVisualization(bool force/*= false*/)
    {
        const auto shouldHave = m_structure && m_visible;
        if (shouldHave != m_visualizationCreated || force)
        {
            if (m_visualizationCreated)
            {
                m_visualizationCreated = false;
                handleDestroyVisualization();
            }

            if (shouldHave)
            {
                m_visualizationCreated = true;
                handleCreateVisualization();
            }
        }
    }

    void SceneContentNode::handleAttachedToStructure()
    {
        ASSERT(m_structure != nullptr);
        conditionalCreateVisualization();
    }

    void SceneContentNode::handleDetachedFromStructure()
    {
        ASSERT(m_structure != nullptr);
        conditionalDestroyVisualization();
    }

    void SceneContentNode::handleVisibilityChanged()
    {
        conditionalRecreateVisualization();
    }

    void SceneContentNode::handleCreateVisualization()
    {
        // done in sub classes
    }

    void SceneContentNode::handleDestroyVisualization()
    {
        // done in sub classes
    }

    void SceneContentNode::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentFileNode);
    RTTI_END_TYPE();

    SceneContentFileNode::SceneContentFileNode(const StringBuf& name, ManagedFileNativeResource* file, bool prefab)
        : SceneContentNode(prefab ? SceneContentNodeType::PrefabRoot : SceneContentNodeType::LayerFile, name)
        , m_file(file)
    {}

    void SceneContentFileNode::reloadContent()
    {
        detachAllChildren();

        if (m_file)
        {
            if (auto prefabContent = rtti_cast<game::Prefab>(m_file->loadContent()))
            {
                InplaceArray<SceneContentEntityNodePtr, 10> rootEntities;
                UnpackNodeContainer(*prefabContent->content(), Transform::IDENTITY(), &rootEntities, nullptr);

                TRACE_INFO("Loaded {} root nodes from prefab {}", rootEntities.size(), m_file->depotPath());

                for (const auto& root : rootEntities)
                    attachChildNode(root);
            }
        }
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentDirNode);
    RTTI_END_TYPE();

    SceneContentDirNode::SceneContentDirNode(const StringBuf& name)
        : SceneContentNode(SceneContentNodeType::LayerDir, name)
    {}

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentPrefabRootNode);
    RTTI_END_TYPE();

    SceneContentPrefabRootNode::SceneContentPrefabRootNode(ManagedFileNativeResource* prefabAssetFile)
        : SceneContentFileNode(StringBuf("Prefab"), prefabAssetFile, true)
    {} 

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldRootNode);
    RTTI_END_TYPE();

    SceneContentWorldRootNode::SceneContentWorldRootNode()
        : SceneContentNode(SceneContentNodeType::WorldRoot, "World")
    {}

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentEntityNode);
    RTTI_END_TYPE();

    SceneContentEntityNode::SceneContentEntityNode(const StringBuf& name, const Transform& localToParent, const Array<game::NodeTemplatePtr>& templates, bool originalContent)
        : SceneContentNode(SceneContentNodeType::Entity, name)
        , m_localToParent(localToParent)
        , m_originalContent(originalContent)
    {
        cacheTransformData();
        CompilePayloads(templates, this, m_payloads);
        refreshContentFlags();
    }

    SceneContentEntityNodeDataPtr SceneContentEntityNode::findPayload(StringID name) const
    {
        for (const auto& data : m_payloads)
            if (data->name == name)
                return data;

        return nullptr;
    }

    void SceneContentEntityNode::displayText(IFormatStream& txt) const
    {
        TBaseClass::displayText(txt);

        if (!m_hasLocalContent && !m_hasBaseContent)
        {
            if (m_payloads.size() == 1)
            {
                txt << " [tag:#888]Empty[/tag]";
            }
            else
            {
                txt << " [tag:#888]No data[/tag]";
            }
        }
        else
        {
            if (m_hasBaseContent)
                txt << " [tag:#888]Overriden[/tag]";

            if (m_hasLocalContent)
                txt << " [tag:#88A]Content[/tag]";
        }
    }

    void SceneContentEntityNode::refreshContentFlags()
    {
        m_hasLocalContent = false;
        m_hasBaseContent = false;

        for (const auto& payload : m_payloads)
        {
            m_hasLocalContent |= (payload->editableData != nullptr);
            m_hasBaseContent |= (payload->baseData != nullptr);
        }
    }

    void SceneContentEntityNode::invalidatePayload()
    {

    }

    void SceneContentEntityNode::attachPayload(SceneContentEntityNodeData* payload)
    {
        DEBUG_CHECK_RETURN(payload);
        DEBUG_CHECK_RETURN(payload->baseData == nullptr);
        DEBUG_CHECK_RETURN(payload->editableData);
        DEBUG_CHECK_RETURN(!findPayload(payload->name));
        DEBUG_CHECK_RETURN(!m_payloads.contains(payload));

        m_payloads.pushBack(AddRef(payload));
        invalidatePayload();
        refreshContentFlags();
    }

    void SceneContentEntityNode::detachPayload(SceneContentEntityNodeData* payload)
    {
        DEBUG_CHECK_RETURN(payload);
        DEBUG_CHECK_RETURN(m_payloads.contains(payload));

        m_payloads.remove(payload);
        invalidatePayload();
        refreshContentFlags();
    }

    void SceneContentEntityNode::CompilePayloads(const Array<game::NodeTemplatePtr>& templates, SceneContentEntityNode* owner, Array<SceneContentEntityNodeDataPtr>& outPayloads)
    {
        // entity payload
        {
            auto payload = CreateSharedPtr<SceneContentEntityNodeData>();

            if (templates.size() >= 1)
            {
                for (auto i : templates.indexRange().reversed())
                {
                    const auto& nodeTemplate = templates[i];
                    const auto data = CloneObject(static_cast<IObjectTemplate*>(nodeTemplate->entityTemplate()));

                    if (i == templates.lastValidIndex())
                    {
                        payload->editableData = data;
                    }
                    else if (data)
                    {
                        data->rebase(payload->baseData);
                        data->detach();
                        payload->baseData = data;
                    }
                }
            }

            // NOTE: we create entity payload even if we have no source data templates
            outPayloads.pushBack(payload);
        }

        // collect names of all components 
        struct Entry
        {
            StringID name;
            game::ComponentTemplatePtr data;
            bool content = false;
        };

        HashMap<StringID, Array<Entry>> componentData;
        for (const auto& nodeTemplate : templates)
        {
            for (const auto& data : nodeTemplate->componentTemplates())
            {
                if (data.name && data.data)
                {
                    Entry entry;
                    entry.name = data.name;
                    entry.data = data.data;
                    entry.content = (nodeTemplate == templates.back());
                    componentData[data.name].pushBack(entry);
                }
            }
        }

        // create data payloads for components
        for (const auto& it : componentData.values())
        {
            auto payload = CreateSharedPtr<SceneContentEntityNodeData>();
            payload->name = it.back().name;

            for (auto i : it.indexRange().reversed())
            {
                const auto& entry = it[i];
                const auto data = CloneObject(static_cast<IObjectTemplate*>(entry.data));

                if (entry.content)
                {
                    payload->editableData = data;
                }
                else if (data)
                {
                    data->rebase(payload->baseData);
                    data->detach();
                    payload->baseData = data;
                }
            }

            outPayloads.pushBack(payload);
        }

        // rebase payloads
        for (auto& payload : outPayloads)
        {
            if (payload->editableData && payload->baseData)
            {
                payload->editableData->rebase(payload->baseData);
            }
        }
    }

    void SceneContentEntityNode::handleCreateVisualization()
    {
        // TODO: create streaming proxy
    }

    void SceneContentEntityNode::handleDestroyVisualization()
    {

    }

    void SceneContentEntityNode::handleParentChanged()
    {
        cacheTransformData();
    }

    void SceneContentEntityNode::changeLocalPlacement(const Transform& newTramsform, bool force /*= false*/)
    {        
        if (m_localToParent != newTramsform || force)
        {
            m_localToParent = newTramsform;
            updateTransform(force);
        }
    }

    void SceneContentEntityNode::updateTransform(bool force /*= false*/, bool recursive /*= true*/)
    {
        if (cacheTransformData() || force)
        {
            if (recursive)
            {
                for (const auto& child : entities())
                    child->updateTransform(force, recursive);
            }
        }
    }

    bool SceneContentEntityNode::cacheTransformData()
    {
        const auto prev = m_cachedLocalToWorldTransform;

        if (auto parentEntity = rtti_cast<SceneContentEntityNode>(parent()))
            m_cachedLocalToWorldTransform = parentEntity->cachedLocalToWorldTransform() * m_localToParent;
        else
            m_cachedLocalToWorldTransform = AbsoluteTransform::ROOT() * m_localToParent;

        m_cachedLocalToWorldMatrix = m_cachedLocalToWorldTransform.approximate();

        return prev != m_cachedLocalToWorldTransform;
    }

    void SceneContentEntityNode::handleTransformUpdated()
    {
        // implemented in children
    }

    void SceneContentEntityNode::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        // TEMPSHIT
        rendering::scene::DebugLineDrawer dd(frame.geometry.solid);
        dd.axes(cachedLocalToWorldMatrix(), 0.1f);
    }

    //---

    static bool ValidateNode(const game::NodeTemplateCompiledData* data)
    {
        DEBUG_CHECK_RETURN_V(data, false);

        if (data->templates.empty())
            return false;

        for (const auto& ptr : data->templates)
            if (ptr->type() == game::NodeTemplateType::Content)
                return true;

        return false;
    }

    static void ExtractNodes(const game::NodeTemplateContainer* originalContainer, const Transform* additionalRootPlacement, SceneContentEntityNode* parent, game::NodeTemplateCompiledData* data, Array<SceneContentEntityNodePtr>* outRootEntities, Array<SceneContentEntityNodePtr>* outAllEntities)
    {
        // no content
        if (!ValidateNode(data))
            return;

        // get the list of templates in the node
        auto templates = std::move(data->templates);
        DEBUG_CHECK_RETURN(!templates.empty()); // illegal

        // transform roots
        const auto nodePlacement = additionalRootPlacement ? data->localToParent.applyTo(*additionalRootPlacement) : data->localToParent;

        // is this original content node ?
        const auto originalContent = (templates.size() == 1) && (templates[0]->hasParent(originalContainer));

        // create the editor node
        auto name = StringBuf(data->name.view());
        auto node = CreateSharedPtr<SceneContentEntityNode>(name, nodePlacement, templates, originalContent);
        parent->attachChildNode(node);

        // create our own children
        for (const auto& child : data->children)
            ExtractNodes(originalContainer, nullptr, node, child, outRootEntities, outAllEntities);

        if (!parent && outRootEntities)
            outRootEntities->pushBack(node);
        if (outAllEntities)
            outAllEntities->pushBack(node);
    }

    void UnpackNodeContainer(const game::NodeTemplateContainer& nodeContainer, const Transform& additionalRootPlacement, Array<SceneContentEntityNodePtr>* outRootEntities, Array<SceneContentEntityNodePtr>* outAllEntities)
    {
        // compile all nodes
        auto compiledNodes = CreateSharedPtr<game::NodeTemplateCompiledOutput>();
        nodeContainer.compile(*compiledNodes, nullptr);

        // extract nodes
        for (const auto& root : compiledNodes->roots)
            ExtractNodes(&nodeContainer, &additionalRootPlacement, nullptr, root, outRootEntities, outAllEntities);
    }
    
    //---

} // ed