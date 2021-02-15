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

#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

#include "base/editor/include/managedFileNativeResource.h"
#include "base/resource/include/objectIndirectTemplate.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/world/include/worldNodeTemplate.h"
#include "base/world/include/worldPrefab.h"
#include "base/resource/include/objectIndirectTemplateCompiler.h"
#include "base/object/include/rttiResourceReferenceType.h"
#include "base/world/include/worldEntity.h"

namespace ed
{
    
    //---

    static const int MAX_DEPTH = 10;

    static EulerTransform MergeTransforms(const EulerTransform& base, const EulerTransform& cur)
    {
        // handle most common case - no transform
        if (base.isIdentity())
            return cur;
        else if (cur.isIdentity())
            return base;

        EulerTransform ret;
        ret.T = base.T + cur.T;
        ret.R = base.R + cur.R;
        ret.S = base.S * cur.S;
        return ret;
    }

    static EulerTransform MergeTransforms(const Array<const ObjectIndirectTemplate*>& templates)
    {
        EulerTransform ret;
        bool first = true;

        for (const auto& data : templates)
        {
            if (data->enabled())
            {
                if (!data->placement().isIdentity())
                {
                    if (first)
                        ret = data->placement();
                    else
                        ret = MergeTransforms(ret, data->placement());

                    first = false;
                }
            }
        }

        return ret;
    }

    static EulerTransform MergeTransforms(const Array<ObjectIndirectTemplatePtr>& templates)
    {
        EulerTransform ret;
        bool first = true;

        for (const auto& data : templates)
        {
            if (data->enabled())
            {
                if (!data->placement().isIdentity())
                {
                    if (first)
                        ret = data->placement();
                    else
                        ret = MergeTransforms(ret, data->placement());

                    first = false;
                }
            }
        }

        return ret;
    }

    static EulerTransform MergeTransforms(const Array<const world::NodeTemplate*>& templates)
    {
        EulerTransform ret;
        bool first = true;

        for (const auto& data : templates)
        {
            if (data->m_entityTemplate && data->m_entityTemplate->enabled())
            {
                if (!data->m_entityTemplate->placement().isIdentity())
                {
                    if (first)
                        ret = data->m_entityTemplate->placement();
                    else
                        ret = MergeTransforms(ret, data->m_entityTemplate->placement());

                    first = false;
                }
            }
        }

        return ret;
    }

    static EulerTransform MergeTransforms(const Array<world::NodeTemplatePtr>& templates)
    {
        EulerTransform ret;
        bool first = true;

        for (const auto& data : templates)
        {
            if (data->m_entityTemplate && data->m_entityTemplate->enabled())
            {
                if (!data->m_entityTemplate->placement().isIdentity())
                {
                    if (first)
                        ret = data->m_entityTemplate->placement();
                    else
                        ret = MergeTransforms(ret, data->m_entityTemplate->placement());

                    first = false;
                }
            }
        }

        return ret;
    }

    EulerTransform MakeTransformRelative(const EulerTransform& cur, const EulerTransform& base)
    {
        EulerTransform ret = cur;
        ret.T -= base.T;
        ret.R -= base.R;
        ret.S /= base.S;
        return ret;
    }
    
    //---

    RTTI_BEGIN_TYPE_ENUM(SceneContentNodeType);
        RTTI_ENUM_OPTION(None);
        RTTI_ENUM_OPTION(LayerFile);
        RTTI_ENUM_OPTION(LayerDir);
        RTTI_ENUM_OPTION(WorldRoot);
        RTTI_ENUM_OPTION(PrefabRoot);
        RTTI_ENUM_OPTION(Entity);
        RTTI_ENUM_OPTION(Behavior);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentNode);
    RTTI_END_TYPE();

    SceneContentNode::SceneContentNode(SceneContentNodeType type, const StringBuf& name)
        : m_type(type)
        , m_name(name)
    {
        m_uniqueModelIndex = ui::ModelIndex::AllocateUniqueIndex();
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

    StringView SceneContentNode::IconTextForType(SceneContentNodeType type)
    {
        switch (type)
        {
            case SceneContentNodeType::Entity: return "[img:entity]";
            case SceneContentNodeType::Behavior: return "[img:component]";
            case SceneContentNodeType::LayerFile: return "[img:page]";
            case SceneContentNodeType::LayerDir: return "[img:folder]";
            case SceneContentNodeType::PrefabRoot: return "[img:brick]";
            case SceneContentNodeType::WorldRoot: return "[img:world]";
        }

        return "";
    }

    void SceneContentNode::displayText(IFormatStream& txt) const
    {
        if (m_visualFlags.test(SceneContentNodeVisualBit::ActiveNode))
            txt << "[b][i]";

        txt << IconTextForType(m_type);
        txt << " ";
        txt << m_name;

        if (m_modified)
            txt << "*";

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
            m_structure->nodeAdded(this);

            for (const auto& child : m_children)
                child->conditionalAttachToStructure();
        }
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
        else if (auto component = rtti_cast<SceneContentBehaviorNode>(child))
        {
            m_behaviors.pushBack(AddRef(component));
        }

        child->conditionalAttachToStructure();

        handleChildAdded(child);

        markModified();
    }

    void SceneContentNode::detachAllChildren()
    {
        const auto children = m_children;

        for (auto i : children.indexRange().reversed())
            detachChildNode(children[i]);

        m_childrenNames.clear();

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

        if (auto component = rtti_cast<SceneContentBehaviorNode>(child))
            m_behaviors.remove(component);

        if (child->m_structure)
            child->conditionalDetachFromStructure();

        m_childrenNames.clear();

        handleChildRemoved(child); // call the notification while we still have a valid parent

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
    }

    void SceneContentNode::handleDebugRender(rendering::scene::FrameParams& frame) const
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

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentDataNode);
    RTTI_END_TYPE();

    SceneContentDataNode::SceneContentDataNode(SceneContentNodeType nodeType, const StringBuf& name, const ObjectIndirectTemplate* editableData)
        : SceneContentNode(nodeType, name)
        , m_editableData(AddRef(editableData))
        , m_dataEvents(this)
    {
        ASSERT(m_editableData != nullptr);
        m_editableData->parent(this);

        m_dataEvents.bind(m_editableData->eventKey(), EVENT_OBJECT_PROPERTY_CHANGED) = [this](StringBuf path)
        {
            handleDataPropertyChanged(path);
        };

        cacheTransformData();
    }

    void SceneContentDataNode::displayText(IFormatStream& txt) const
    {
        TBaseClass::displayText(txt);

        if (auto ent = rtti_cast<SceneContentEntityNode>(this))
            if (!ent->localPrefabs().empty())
                txt << " [tag:#4AA]Prefab[/tag]";

        if (!m_baseData.empty() && !m_editableData->properties().empty())
            txt << " [tag:#888]Override[/tag]";

        auto inheritedClass = dataClass();

        if (inheritedClass && m_editableData->templateClass() && inheritedClass != m_editableData->templateClass())
            txt << " [tag:#D44]Class Change[/tag]";

        if (m_editableData->templateClass())
        {
            auto className = m_editableData->templateClass().name().view();
            className = className.afterLastOrFull("::");
            className = className.beforeFirstOrFull("Template");
            txt.appendf(" [i]({})[/i]", className);
        }
        else if (inheritedClass)
        {
            auto className = inheritedClass.name().view();
            className = className.afterLastOrFull("::");
            className = className.beforeFirstOrFull("Template");
            txt.appendf(" [i][color:#888]({})[/color][/i]", className);
        }
    }

    void SceneContentDataNode::updateBaseTemplates(const Array<const ObjectIndirectTemplate*>& baseData)
    {
        m_baseData.reset();
        m_baseData.reserve(baseData.size());
        for (const auto* base : baseData)
            m_baseData.pushBack(AddRef(base));

        m_baseLocalToTransform = MergeTransforms(m_baseData);

        cacheTransformData();
    }

    void SceneContentDataNode::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        // TEMPSHIT
        rendering::scene::DebugDrawer dd(frame.geometry.solid);
        dd.axes(cachedLocalToWorldMatrix(), 0.1f);
    }

    void SceneContentDataNode::handleParentChanged()
    {
        cacheTransformData();
    }

    void SceneContentDataNode::handleDataPropertyChanged(const StringBuf& path)
    {
        
    }

    ClassType SceneContentDataNode::dataClass() const
    {
        if (m_editableData)
            if (m_editableData->templateClass())
                return m_editableData->templateClass();

        for (auto index : m_baseData.indexRange().reversed())
            if (m_baseData[index]->templateClass())
                return m_baseData[index]->templateClass();

        return nullptr;
    }

    void SceneContentDataNode::changeClass(ClassType templateClass)
    {
        if (m_editableData->templateClass() != templateClass)
        {
            m_editableData->templateClass(templateClass);

            markDirty(SceneContentNodeDirtyBit::Content);

            {
                SceneContentNodePtr selfPtr(AddRef(static_cast<SceneContentNode*>(this)));

                postEvent(EVENT_CONTENT_NODE_RENAMED, selfPtr);

                if (structure())
                    structure()->postEvent(EVENT_CONTENT_STRUCTURE_NODE_RENAMED, selfPtr);
            }
        }
    }

    ObjectIndirectTemplatePtr SceneContentDataNode::compileFlatData() const
    {
        ObjectIndirectTemplateCompiler compiler;
        for (const auto& ptr : baseData())
            compiler.addTemplate(ptr);
        compiler.addTemplate(editableData());

        return compiler.flatten();
    }

    static const float NodeTransform_MinDT = 0.001f;
    static const float NodeTransform_MinDR = 0.001f;
    static const float NodeTransform_MinDS = 0.001f;

    static void SanitizeTransform(EulerTransform& ret)
    {
        if (std::fabs(ret.T.x) <= NodeTransform_MinDT)
            ret.T.x = 0.0f;
        if (std::fabs(ret.T.y) <= NodeTransform_MinDT)
            ret.T.y = 0.0f;
        if (std::fabs(ret.T.z) <= NodeTransform_MinDT)
            ret.T.z = 0.0f;

        if (std::fabs(ret.R.pitch) <= NodeTransform_MinDR)
            ret.R.pitch = 0.0f;
        if (std::fabs(ret.R.yaw) <= NodeTransform_MinDR)
            ret.R.yaw = 0.0f;
        if (std::fabs(ret.R.roll) <= NodeTransform_MinDR)
            ret.R.roll = 0.0f;

        if (std::fabs(ret.S.x - 1.0f) <= NodeTransform_MinDS)
            ret.S.x = 1.0f;
        if (std::fabs(ret.S.y - 1.0f) <= NodeTransform_MinDS)
            ret.S.y = 1.0f;
        if (std::fabs(ret.S.z - 1.0f) <= NodeTransform_MinDS)
            ret.S.z = 1.0f;
    }

    void SceneContentDataNode::changeLocalPlacement(const EulerTransform& newLocalToParent, bool force /*= false*/)
    {
        auto rebasedLocalToWorld = MakeTransformRelative(newLocalToParent, m_baseLocalToTransform);

        SanitizeTransform(rebasedLocalToWorld);

        if (m_editableData->placement() != rebasedLocalToWorld || force)
        {
            m_editableData->placement(rebasedLocalToWorld);
            cacheTransformData();
            markModified();
        }
    }

    const AbsoluteTransform& SceneContentDataNode::cachedParentToWorldTransform() const
    {
        if (auto parent = rtti_cast<SceneContentDataNode>(this->parent()))
            return parent->cachedLocalToWorldTransform();

        return AbsoluteTransform::ROOT();
    }

    void SceneContentDataNode::cacheTransformData()
    {
        m_localToWorld = MergeTransforms(m_baseLocalToTransform, m_editableData->placement());

        auto newPlacement = cachedParentToWorldTransform() * m_localToWorld.toTransform();
        if (newPlacement != m_cachedLocalToWorldPlacement)
        {
            m_cachedLocalToWorldPlacement = newPlacement;
            m_cachedLocalToWorldMatrix = m_cachedLocalToWorldPlacement.approximate();

            handleTransformUpdated();

            for (const auto& child : children())
                if (auto dataNode = rtti_cast<SceneContentDataNode>(child.get()))
                    dataNode->cacheTransformData();
        }
    }

    void SceneContentDataNode::handleTransformUpdated()
    {
        markDirty(SceneContentNodeDirtyBit::Transform);
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentEntityNode);
    RTTI_END_TYPE();

    static Array<const ObjectIndirectTemplate*> ExtractEntityData(const Array<world::NodeTemplatePtr>& sourceNodeData)
    {
        Array<const ObjectIndirectTemplate*> ret;
        ret.reserve(sourceNodeData.size());

        for (const auto& ptr : sourceNodeData)
            if (ptr && ptr->m_entityTemplate)
                ret.pushBack(ptr->m_entityTemplate);
        
        return ret;
    }

    TYPE_TLS uint32_t GPrefabInstancingDepth = 0;

    SceneContentEntityNode::SceneContentEntityNode(const StringBuf& name, const world::NodeTemplatePtr& node, const Array<world::NodeTemplatePtr>& inheritedTemplates)
        : SceneContentDataNode(SceneContentNodeType::Entity, name, node ? node->m_entityTemplate : RefNew<ObjectIndirectTemplate>())
        , m_inheritedTemplates(inheritedTemplates)
    {
        GPrefabInstancingDepth += 1;

        if (GPrefabInstancingDepth < 10)
        {
            // apply initial instanced content - initial set of components and children
            SceneContentEntityInstancedContent instancedContent;
            createInstancedContent(node, instancedContent);
            applyInstancedContent(instancedContent);
        }

        GPrefabInstancingDepth -= 1;
    }

    void SceneContentEntityNode::extractCurrentInstancedContent(SceneContentEntityInstancedContent& outInstancedContent)
    {
        outInstancedContent.childNodes = children();
        outInstancedContent.localPrefabs = std::move(m_localPrefabs);

        m_localPrefabs.reset();

        detachAllChildren();
    }

    static void SuckInPrefab(uint64_t nodeSeed, const world::NodeTemplatePrefabSetup& prefabEntry, HashSet<const world::Prefab*>& allVisitedPrefabs, Array<const world::NodeTemplate*>& outPrefabRoots)
    {
        if (!prefabEntry.enabled)
            return;

        auto loadedPrefab = prefabEntry.prefab.acquire();
        if (!loadedPrefab)
            return;

        if (!allVisitedPrefabs.insert(loadedPrefab))
            return;

        auto prefabRootNode = loadedPrefab->root();
        if (!prefabRootNode)
            return;

        for (const auto& rootPrefab : prefabRootNode->m_prefabAssets)
            SuckInPrefab(nodeSeed, rootPrefab, allVisitedPrefabs, outPrefabRoots);

        outPrefabRoots.pushBack(prefabRootNode);
    }

    void SceneContentEntityNode::collectBaseNodes(const Array<world::NodeTemplatePrefabSetup>& localPrefabs, Array<const world::NodeTemplate*>& outBaseNodes) const
    {
        const auto thisNodeSeed = 0;

        outBaseNodes.reset();

        // collect prefabs from inherited nodes
        InplaceArray<const world::NodeTemplate*, 20> baseNodes;
        HashSet<const world::Prefab*> visitedPrefabs;
        for (const auto& temp : m_inheritedTemplates)
        {
            for (const auto& prefabAsset : temp->m_prefabAssets)
                SuckInPrefab(thisNodeSeed, prefabAsset, visitedPrefabs, outBaseNodes);

            if (!outBaseNodes.contains(temp))
                outBaseNodes.pushBack(temp);
        }

        // collect local prefabs
        for (const auto& localPrefab : localPrefabs)
            SuckInPrefab(thisNodeSeed, localPrefab, visitedPrefabs, outBaseNodes);
    }

    void SceneContentEntityNode::updateBaseTemplates()
    {
        InplaceArray<const world::NodeTemplate*, 10> baseNodes;
        collectBaseNodes(m_localPrefabs, baseNodes);

        InplaceArray<const ObjectIndirectTemplate*, 10> localBaseTemplates;
        for (const auto* node : baseNodes)
            if (node->m_entityTemplate && node->m_entityTemplate->enabled())
                localBaseTemplates.pushBack(node->m_entityTemplate);

        TBaseClass::updateBaseTemplates(localBaseTemplates);
    }

    void SceneContentEntityNode::applyInstancedContent(const SceneContentEntityInstancedContent& content)
    {
        detachAllChildren();

        m_localPrefabs = content.localPrefabs;

        updateBaseTemplates();

        for (const auto& child : content.childNodes)
            attachChildNode(child);
    }

    void SceneContentEntityNode::createInstancedContent(const world::NodeTemplate* dataTemplate, SceneContentEntityInstancedContent& outContent) const
    {
        const auto thisNodeSeed = 0;

        // reset
        outContent.childNodes.reset();
        outContent.localPrefabs.reset();
         
        if (dataTemplate)
            outContent.localPrefabs = dataTemplate->m_prefabAssets;
        
        // collect the entity template bases - first from the inherited nodes, second from local prefabs
        InplaceArray<const world::NodeTemplate*, 10> baseContentNodes;
        collectBaseNodes(outContent.localPrefabs, baseContentNodes);

        // create components
        // NOTE: we have cheaper short-circuit path for most common case of nodes without base
        if (baseContentNodes.empty())
        {
            if (dataTemplate)
            {
                for (const auto& info : dataTemplate->m_behaviorTemplates)
                {
                    if (info.data && info.name)
                    {
                        auto componentNode = RefNew<SceneContentBehaviorNode>(StringBuf(info.name.view()), info.data);
                        outContent.childNodes.pushBack(componentNode);
                    }
                }
            }
        }
        else
        {
            HashMap<StringID, Array<const ObjectIndirectTemplate*>> componentMap;
            HashMap<StringID, ObjectIndirectTemplatePtr> localComponentMap;

            // collect data from base templates to know the super set of all possible components we have
            for (const auto* temp : baseContentNodes)
                for (const auto& comp : temp->m_behaviorTemplates)
                    if (comp.data && comp.name)
                        componentMap[comp.name].pushBack(comp.data);

            // add local components
            if (dataTemplate)
            {
                localComponentMap.reserve(dataTemplate->m_behaviorTemplates.size());
                for (const auto& comp : dataTemplate->m_behaviorTemplates)
                {
                    if (comp.data && comp.name)
                    {
                        localComponentMap[comp.name] = comp.data;
                        componentMap[comp.name].reserve(1);
                    }
                }
            }

            // look at all components that we want to have in this entity and create the necessary scene nodes
            // NOTE: some components may NOT be locally defined and have no local data, for them create the empty data objects (they will be discarded on save)
            for (const auto& pair : componentMap.pairs())
            {
                // if we don't have local data to edit create some
                auto editableComponentData = localComponentMap[pair.key];
                if (!editableComponentData)
                    editableComponentData = RefNew<ObjectIndirectTemplate>();

                // create the component node
                auto componentNode = RefNew<SceneContentBehaviorNode>(StringBuf(pair.key.view()), editableComponentData, pair.value);
                outContent.childNodes.pushBack(componentNode);
            }
        }

        // TODO: run dynamic scripts
        // NOTE: this may create dynamic components and dynamic child entities

        // create children
        // NOTE: we have cheaper short-circuit path for most common case of nodes without base
        if (baseContentNodes.empty())
        {
            // look at local children list
            if (dataTemplate)
            {
                for (const auto& child : dataTemplate->m_children)
                {
                    if (child && child->m_name)
                    {
                        auto componentNode = RefNew<SceneContentEntityNode>(StringBuf(child->m_name.view()), child);
                        outContent.childNodes.pushBack(componentNode);
                    }
                }
            }
        }
        else
        {
            // collect names and data of child nodes
            HashMap<StringID, Array<world::NodeTemplatePtr>> childMap;
            HashMap<StringID, world::NodeTemplatePtr> localChildrenMap;

            // collect data from base templates to know the super set of all possible children we have
            for (const auto* temp : baseContentNodes)
                for (const auto& comp : temp->m_children)
                    if (comp && comp->m_name)
                        childMap[comp->m_name].pushBack(comp);

            // add local components
            if (dataTemplate)
            {
                localChildrenMap.reserve(dataTemplate->m_children.size());
                for (const auto& comp : dataTemplate->m_children)
                {
                    if (comp && comp->m_name)
                    {
                        localChildrenMap[comp->m_name] = comp;
                        childMap[comp->m_name].reserve(1);
                    }
                }
            }

            // look at all children that we want to have in this entity and create the necessary scene nodes
            // NOTE: some children may NOT be locally defined and have no local data, for them create the empty data objects (they will be discarded on save)
            for (const auto& pair : childMap.pairs())
            {
                auto editableChildrenData = localChildrenMap[pair.key];

                // create the component node
                auto childNode = RefNew<SceneContentEntityNode>(StringBuf(pair.key.view()), editableChildrenData, pair.value);
                outContent.childNodes.pushBack(childNode);
            }
        }
    }

    bool SceneContentEntityNode::canAttach(SceneContentNodeType type) const
    {
        return type == SceneContentNodeType::Entity || type == SceneContentNodeType::Behavior;
    }

    bool SceneContentEntityNode::canDelete() const
    {
        auto p = parent();
        if (p && p->type() == SceneContentNodeType::PrefabRoot)
            return false;

        return m_inheritedTemplates.empty(); // we can't delete nodes that are created by instanced prefabs
    }

    bool SceneContentEntityNode::canCopy() const
    {
        return true;
    }

    bool SceneContentEntityNode::canExplodePrefab() const
    {
        return m_inheritedTemplates.empty() && !m_localPrefabs.empty();
    }

    void SceneContentEntityNode::handleChildAdded(SceneContentNode* child)
    {
        TBaseClass::handleChildAdded(child);

        if (child->type() == SceneContentNodeType::Behavior)
            markDirty(SceneContentNodeDirtyBit::Content);
    }

    void SceneContentEntityNode::handleChildRemoved(SceneContentNode* child)
    {
        TBaseClass::handleChildRemoved(child);

        if (child->type() == SceneContentNodeType::Behavior)
            markDirty(SceneContentNodeDirtyBit::Content);
    }
    void SceneContentEntityNode::invalidateData()
    {
        markDirty(SceneContentNodeDirtyBit::Content);
    }

    void SceneContentEntityNode::handleDataPropertyChanged(const StringBuf& data)
    {
        invalidateData();
    }

    void SceneContentEntityNode::handleVisibilityChanged()
    {
        TBaseClass::handleVisibilityChanged();
        markDirty(SceneContentNodeDirtyBit::Visibility);
    }

    world::NodeTemplatePtr SceneContentEntityNode::compileSnapshot() const
    {
        auto ret = RefNew<world::NodeTemplate>();
        ret->m_name = StringID(name());
        
        ret->m_entityTemplate = compileFlatData();
        ret->m_entityTemplate->parent(ret);
        
        for (const auto& beh : behaviors())
        {
            if (!beh->name().empty())
            {
                auto& entry = ret->m_behaviorTemplates.emplaceBack();
                entry.name = StringID(beh->name());
                entry.data = beh->compileFlatData();
                entry.data->parent(ret);
            }
        }

        return ret;
    }

    static bool HasLocalData(const SceneContentDataNode* node)
    {
        DEBUG_CHECK(node->editableData());

        if (node->baseData().empty())
            return true;

        if (!node->editableData()->templateClass().empty())
            return true;

        if (!node->editableData()->properties().empty())
            return true;

        if (!node->editableData()->placement().isIdentity())
            return true;

        return false;
    }

    world::NodeTemplatePtr SceneContentEntityNode::compileDifferentialData() const
    {
        auto ret = RefNew<world::NodeTemplate>();
        ret->m_name = StringID(name());

        // extract child entities
        for (const auto& childEntity : entities())
        {
            if (const auto childEntityData = childEntity->compileDifferentialData())
            {
                ret->m_children.pushBack(childEntityData);
                childEntityData->parent(ret);
            }
        }

        // extract component data 
        for (const auto& behaviorNode : behaviors())
        {
            if (behaviorNode->name().empty())
                continue;

            if (!HasLocalData(behaviorNode))
                continue;

            auto& entry = ret->m_behaviorTemplates.emplaceBack();
            entry.name = StringID(behaviorNode->name());
            entry.data = CloneObject(behaviorNode->editableData(), ret);
        }

        // extract prefab list
        for (const auto& prefabInfo : m_localPrefabs)
        {
            auto& info = ret->m_prefabAssets.emplaceBack();
            info.appearance = prefabInfo.appearance;
            info.enabled = prefabInfo.enabled;
            info.prefab = prefabInfo.prefab;
        }

        // if we don't have children and local components and carry no data than we don't have to be stored
        if (ret->m_children.empty() && ret->m_behaviorTemplates.empty() && !HasLocalData(this) && ret->m_prefabAssets.empty())
            return nullptr;

        // extract entity data
        ret->m_entityTemplate = CloneObject(editableData(), ret);;
        return ret;
    }    

    world::NodeTemplatePtr SceneContentEntityNode::compiledForCopy() const
    {
        auto delta = compileDifferentialData();

        if (!m_inheritedTemplates.empty())
        {
            InplaceArray<const world::NodeTemplate*, 10> baseTemplates;
            for (const auto& tmp : m_inheritedTemplates)
                baseTemplates.pushBack(tmp);

            if (!delta)
                delta = RefNew<world::NodeTemplate>();

            delta = CompileWithInjectedBaseNodes(delta, baseTemplates);
        }

        return delta;
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentBehaviorNode);
    RTTI_END_TYPE();

    SceneContentBehaviorNode::SceneContentBehaviorNode(const StringBuf& name, const ObjectIndirectTemplate* editableData, const Array<const ObjectIndirectTemplate*>& baseData)
        : SceneContentDataNode(SceneContentNodeType::Behavior, name, editableData)
    {
        updateBaseTemplates(baseData);
    }

    bool SceneContentBehaviorNode::canAttach(SceneContentNodeType type) const
    {
        return false;
    }

    bool SceneContentBehaviorNode::canDelete() const
    {
        return baseData().empty();
    }

    bool SceneContentBehaviorNode::canCopy() const
    {
        return baseData().empty();
    }

    /*world::ComponentTemplatePtr SceneContentBehaviorNode::compileData() const
    {
        if (auto cur = rtti_cast<world::ComponentTemplate>(editableData()))
        {
            auto ret = CloneObject(cur);
            ret->rebase(baseData());
            ret->detach();
            ret->placement(calcLocalToParent());
            return ret;
        }

        return nullptr;
    }

    world::ComponentTemplatePtr SceneContentBehaviorNode::compileDifferentialData(bool& outAnyMeaningfulData) const
    {
        auto componentData = rtti_cast<world::ComponentTemplate>(editableData());
        DEBUG_CHECK_RETURN_V(componentData, nullptr);

        auto ret = CloneObject(componentData);

        if (auto base = rtti_cast<world::ComponentTemplate>(baseData()))
        {
            ret->placement(MakeTransformRelative(calcLocalToParent(), base->placement()));
            outAnyMeaningfulData |= IsDataNeeded(*ret);
        }
        else
        {
            ret->placement(calcLocalToParent());
            outAnyMeaningfulData = true;
        }

        return ret;
    }*/

    void SceneContentBehaviorNode::handleDataPropertyChanged(const StringBuf& data)
    {
        if (auto entity = rtti_cast<SceneContentEntityNode>(parent()))
            entity->markDirty(SceneContentNodeDirtyBit::Content);
    }

    void SceneContentBehaviorNode::handleTransformUpdated()
    {
        TBaseClass::handleTransformUpdated();

        if (auto entity = rtti_cast<SceneContentEntityNode>(parent()))
            entity->markDirty(SceneContentNodeDirtyBit::Content); // yeah, content, we need to recompile whole entity
    }

    void SceneContentBehaviorNode::handleLocalVisibilityChanged()
    {
        TBaseClass::handleTransformUpdated();

        if (auto entity = rtti_cast<SceneContentEntityNode>(parent()))
            entity->markDirty(SceneContentNodeDirtyBit::Content); // yeah, content, we need to recompile whole entity
    }

    //---
    
    void SceneContentNode::EnumEntityClassesForResource(ClassType resourceClass, Array<SpecificClassType<world::Entity>>& outEntityClasses)
    {
        InplaceArray<SpecificClassType<world::Entity>, 100> allComponentClasses;
        RTTI::GetInstance().enumClasses(allComponentClasses);

        for (const auto compClass : allComponentClasses)
        {
            for (const auto& templateProp : compClass->allTemplateProperties())
            {
                if (templateProp.type->metaType() == rtti::MetaType::AsyncResourceRef && templateProp.editorData.m_primaryResource)
                {
                    const auto* asyncRefType = static_cast<const rtti::IResourceReferenceType*>(templateProp.type.ptr());
                    if (asyncRefType->referenceResourceClass().is(resourceClass))
                    {
                        outEntityClasses.pushBack(compClass);
                        break;
                    }
                }
            }
        }
    }

    //--

} // ed