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
#include "sceneContentNodesEntity.h"
#include "sceneContentNodesTree.h"

#include "editor/assets/include/browserService.h"

#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"

#include "core/object/include/rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)
    
//---

RTTI_BEGIN_TYPE_ENUM(SceneContentNodeType);
    RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(LayerFile);
    RTTI_ENUM_OPTION(LayerDir);
    RTTI_ENUM_OPTION(WorldRoot);
    RTTI_ENUM_OPTION(PrefabRoot);
    RTTI_ENUM_OPTION(Entity);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentNode);
RTTI_END_TYPE();

SceneContentNode::SceneContentNode(SceneContentNodeType type, const StringBuf& name)
    : m_type(type)
    , m_name(name)
{
    m_treeItem = RefNew<SceneContentTreeItem>(this);
}

void SceneContentNode::updateDisplayElement()
{
    m_treeItem->updateDisplayState();
}

bool SceneContentNode::contains(const SceneContentNode* node) const
{
    while (node)
    {
        if (node == this)
            return true;
        node = node->parent();
    }

    return false;
}

void SceneContentNode::collectHierarchyNodes(Array<const SceneContentNode*>& outNodes) const
{
    const auto* cur = this;
    while (cur)
    {
        outNodes.pushBack(cur);
        cur = cur->parent();
    }

    std::reverse(outNodes.begin(), outNodes.end());
}

StringBuf SceneContentNode::buildHierarchicalName() const
{
    InplaceArray<const SceneContentNode*, 10> nodes;
    collectHierarchyNodes(nodes);

    StringBuilder txt;
    for (const auto* node : nodes)
    {
        if (node->parent() == nullptr)
            continue;

        txt << "/";
        txt << node->name();
    }

    return txt.toString();
}

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

        markModified();
    }
}

StringBuf SceneContentNode::BuildUniqueName(StringView coreName, bool userGiven, const HashSet<StringBuf>& takenNames)
{
    if (coreName.empty())
        coreName = "node";

    uint32_t counter = 0;
    coreName = coreName.trimTailNumbers(&counter);

    StringBuilder txt;
    do
    {
        txt.clear();
        txt << coreName;
        txt << counter;
        counter += 1;
    } while (takenNames.contains(txt.view()));

    return txt.toString();
}

StringBuf SceneContentNode::buildUniqueName(StringView coreName, bool userGiven, const HashSet<StringBuf>* additionalTakenName) const
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

    if (userGiven && !coreName.empty())
        if (!m_childrenNames.contains(coreName))
            if (!additionalTakenName || !additionalTakenName->contains(coreName))
                return StringBuf(coreName);

    if (coreName.empty())
        coreName = "node";

    uint32_t counter = 0;
    coreName = coreName.trimTailNumbers(&counter);

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
        m_structure->nodeAdded(this);

        for (const auto& child : m_children)
            child->conditionalAttachToStructure();
    }
}

void SceneContentNode::displayTags(IFormatStream& f) const
{
    // nothing
}

void SceneContentNode::markModified()
{
    conditionalChangeModifiedStatus(true);
    TBaseClass::markModified();
}

void SceneContentNode::conditionalChangeModifiedStatus(bool newStatus)
{
    if (m_modified != newStatus)
    {
        m_modified = newStatus;

        SceneContentNodePtr selfRef(AddRef(this));

        if (m_structure)
            m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_MODIFIED_FLAG_CHANGED, selfRef);

        postEvent(EVENT_CONTENT_NODE_MODIFIED_FLAG_CHANGED, selfRef);

        updateDisplayElement();
    }
}

void SceneContentNode::resetDirtyStatus(SceneContentNodeDirtyFlags flags)
{
    m_dirtyFlags = flags;
}

void SceneContentNode::markDirty(SceneContentNodeDirtyFlags flags)
{
    auto oldFlags = m_dirtyFlags;
    m_dirtyFlags |= flags;
    if (m_dirtyFlags != oldFlags)
    {
        if (m_structure)
            m_structure->nodeDirtyContent(this);
    }
}

void SceneContentNode::resetModifiedStatus(bool recursive /*= true*/)
{
    conditionalChangeModifiedStatus(false);

    if (recursive)
        for (const auto& child : m_children)
            child->resetModifiedStatus(recursive);
}

void SceneContentNode::attachChildNode(SceneContentNode* child)
{
    DEBUG_CHECK_RETURN(child);
    DEBUG_CHECK_RETURN(child->parent() == nullptr);
    DEBUG_CHECK_RETURN(child != this); // common mistake
    DEBUG_CHECK_RETURN(!child->contains(this));
    DEBUG_CHECK_RETURN(!contains(child));
    DEBUG_CHECK_RETURN(canAttach(child->type()));

    child->IObject::parent(this);
    child->handleParentChanged();

    m_children.pushBack(AddRef(child));

    if (auto entity = rtti_cast<SceneContentEntityNode>(child))
    {
        m_entities.pushBack(AddRef(entity));
        m_childrenNames.insert(child->name());
    }
    /*else if (auto component = rtti_cast<SceneContentBehaviorNode>(child))
    {
        m_behaviors.pushBack(AddRef(component));
    }*/

    child->conditionalAttachToStructure();

    child->m_treeItem->updateDisplayState();
    m_treeItem->addChild(child->m_treeItem);

    handleChildAdded(child);

    markModified();
}

void SceneContentNode::detachAllChildren()
{
    const auto children = m_children;

    for (auto i : children.indexRange().reversed())
        detachChildNode(children[i]);

    m_childrenNames.clear();

    m_treeItem->removeAllChildren();

    markModified();
}

void SceneContentNode::detachChildNode(SceneContentNode* child)
{
    DEBUG_CHECK_RETURN(child);
    DEBUG_CHECK_RETURN(child->parent() == this);

    SceneContentNodePtr childRef(AddRef(child));

    m_children.remove(child);

    if (auto entity = rtti_cast<SceneContentEntityNode>(child))
        m_entities.remove(entity);

    /*if (auto component = rtti_cast<SceneContentBehaviorNode>(child))
        m_behaviors.remove(component);*/

    if (child->m_structure)
        child->conditionalDetachFromStructure();

    m_childrenNames.clear();

    handleChildRemoved(child); // call the notification while we still have a valid parent

    m_treeItem->removeChild(child->m_treeItem);

    child->IObject::parent(nullptr);
    child->handleParentChanged();

    markModified();
}

SceneContentNode* SceneContentNode::findNodeByPath(StringView path) const
{
    DEBUG_CHECK_RETURN_EX_V(!path.empty(), "Node path should not be empty", nullptr);
    DEBUG_CHECK_RETURN_EX_V(path.beginsWith("/"), "Node path should being with '/'", nullptr);

    InplaceArray<StringView, 20> names;
    path.slice("/", false, names);

    auto* cur = this;
    for (const auto name : names)
    {
        if (!cur)
            break;

        if (name == ".")
            continue;

        if (name == "..")
            cur = cur->parent();
        else
            cur = cur->findChild(name);
    }

    return const_cast<SceneContentNode*>(cur);
}


SceneContentNode* SceneContentNode::findChild(StringView name) const
{
    for (const auto& node : m_children)
        if (node->name() == name)
            return node;

    return nullptr;
}

void SceneContentNode::propagateMergedVisibilityStateFromThis()
{
    auto parentNode = parent();
    auto parentVisibilityState = parentNode ? parentNode->m_visible : true;
    propagateMergedVisibilityState(parentVisibilityState);
}

void SceneContentNode::recalculateVisibility()
{
    propagateMergedVisibilityStateFromThis();
}

void SceneContentNode::propagateMergedVisibilityState(bool parentVisibilityState)
{
    const auto newState = parentVisibilityState && visibilityFlagBool();
    if (newState != m_visible)
    {
        m_visible = newState;
        handleVisibilityChanged();

        for (const auto& child : m_children)
            child->propagateMergedVisibilityState(m_visible);
    }
}

bool SceneContentNode::visibilityFlagBool() const
{
    if (m_localVisibilityFlag == SceneContentNodeLocalVisibilityState::Default) // default is most common
        return defaultVisibilityFlag();
    else
        return m_localVisibilityFlag != SceneContentNodeLocalVisibilityState::Hidden;
}

bool SceneContentNode::defaultVisibilityFlag() const
{
    return true;
}

void SceneContentNode::visibility(SceneContentNodeLocalVisibilityState flag, bool propagateState/* = true*/)
{
    if (flag != m_localVisibilityFlag)
    {
        m_localVisibilityFlag = flag;
        handleLocalVisibilityChanged();

        if (propagateState)
            propagateMergedVisibilityStateFromThis();
    }
}

static void MarkRecrusiveDirty(SceneContentNode* a, SceneContentNodeDirtyBit flag)
{
    a->markDirty(flag);

    for (auto& child : a->children())
        MarkRecrusiveDirty(child, flag);
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
        m_visualFlags.clear(flag);
        changed = true;
    }

    if (changed)
    {
        SceneContentNodePtr selfRef(AddRef(this));

        if (m_structure)
            m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED, selfRef);

        postEvent(EVENT_CONTENT_NODE_VISUAL_FLAG_CHANGED, selfRef);

        updateDisplayElement();
    }

    if (flag == SceneContentNodeVisualBit::SelectedNode)
    {
        if (type() == SceneContentNodeType::Entity)
        {
            //markDirty(SceneContentNodeDirtyBit::Selection);
            MarkRecrusiveDirty(this, SceneContentNodeDirtyBit::Selection);
        }
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
    DEBUG_CHECK_RETURN(!m_children.contains(child)); // should already be gone

    SceneContentNodePtr childRef(AddRef(child));

    if (m_structure)
        m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_REMOVED, childRef);

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
            m_structure->nodeAdded(this);
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

    m_structure->nodeRemoved(this);
    m_structure = nullptr;
}
   
void SceneContentNode::handleLocalVisibilityChanged()
{
    SceneContentNodePtr selfRef(AddRef(this));

    if (m_structure)
        m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_VISIBILITY_CHANGED, selfRef);

    postEvent(EVENT_CONTENT_NODE_VISIBILITY_CHANGED, selfRef);
}

void SceneContentNode::handleVisibilityChanged()
{
    SceneContentNodePtr selfRef(AddRef(this));

    if (m_structure)
        m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_VISIBILITY_CHANGED, selfRef);

    postEvent(EVENT_CONTENT_NODE_VISIBILITY_CHANGED, selfRef);

    updateDisplayElement();
}

void SceneContentNode::handleDebugRender(DebugGeometryCollector& debug) const
{
}


//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldRoot);
RTTI_END_TYPE();

SceneContentWorldRoot::SceneContentWorldRoot()
    : SceneContentNode(SceneContentNodeType::WorldRoot, "World")
{}

bool SceneContentWorldRoot::canAttach(SceneContentNodeType type) const
{
    return type == SceneContentNodeType::LayerDir;
}

bool SceneContentWorldRoot::canDelete() const
{
    return false;
}

bool SceneContentWorldRoot::canCopy() const
{
    return false;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldLayer);
RTTI_END_TYPE();

SceneContentWorldLayer::SceneContentWorldLayer(const StringBuf& name)
    : SceneContentNode(SceneContentNodeType::LayerFile, name)
{}

bool SceneContentWorldLayer::canAttach(SceneContentNodeType type) const
{
    return type == SceneContentNodeType::Entity;
}

bool SceneContentWorldLayer::canDelete() const
{
    return true;
}

bool SceneContentWorldLayer::canCopy() const
{
    return false;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldDir);
RTTI_END_TYPE();

SceneContentWorldDir::SceneContentWorldDir(const StringBuf& name, bool system)
    : SceneContentNode(SceneContentNodeType::LayerDir, name)
    , m_systemDirectory(system)
{}

bool SceneContentWorldDir::defaultVisibilityFlag() const
{
    if (const auto parentLayerDir = rtti_cast<SceneContentWorldDir>(parent()))
        if (parentLayerDir->m_systemDirectory)
            return false;

    return true;
}

bool SceneContentWorldDir::canAttach(SceneContentNodeType type) const
{
    return type == SceneContentNodeType::LayerDir || type == SceneContentNodeType::LayerFile;
}

bool SceneContentWorldDir::canDelete() const
{
    return !m_systemDirectory;
}

bool SceneContentWorldDir::canCopy() const
{
    return false;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentPrefabRoot);
RTTI_END_TYPE();

SceneContentPrefabRoot::SceneContentPrefabRoot()
    : SceneContentNode(SceneContentNodeType::PrefabRoot, StringBuf("Prefab"))
{} 

bool SceneContentPrefabRoot::canAttach(SceneContentNodeType type) const
{
    if (type == SceneContentNodeType::Entity)
        return children().empty();

    return false;
}

bool SceneContentPrefabRoot::canDelete() const
{
    return false;
}

bool SceneContentPrefabRoot::canCopy() const
{
    return false;
}

//---

END_BOOMER_NAMESPACE_EX(ed)
