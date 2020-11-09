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
#include "base/object/include/objectTemplate.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/world/include/worldNodeTemplate.h"
#include "base/world/include/worldEntityTemplate.h"
#include "base/world/include/worldComponentTemplate.h"
#include "base/world/include/worldPrefab.h"
namespace ed
{
    
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentNode);
    RTTI_END_TYPE();

    SceneContentNode::SceneContentNode(SceneContentNodeType type, const StringBuf& name)
        : m_type(type)
        , m_name(name)
    {
        m_uniqueModelIndex = ui::ModelIndex::AllocateUniqueIndex();
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

    StringBuf SceneContentNode::buildUniqueName(StringView<char> coreName, bool userGiven, const HashSet<StringBuf>* additionalTakenName) const
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

    void SceneContentNode::displayText(IFormatStream& txt) const
    {
        if (m_visualFlags.test(SceneContentNodeVisualBit::ActiveNode))
            txt << "[b][i]";

        switch (m_type)
        {
            case SceneContentNodeType::Entity: txt << "[img:entity]"; break;
            case SceneContentNodeType::Component: txt << "[img:component]"; break;
            case SceneContentNodeType::LayerFile: txt << "[img:page]"; break;
            case SceneContentNodeType::LayerDir: txt << "[img:folder]"; break;
            case SceneContentNodeType::PrefabRoot: txt << "[img:brick]"; break;
            case SceneContentNodeType::WorldRoot: txt << "[img:world]"; break;
        }

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
                child->resetModifiedStatus(false);
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

        if (auto component = rtti_cast<SceneContentComponentNode>(child))
            m_components.pushBack(AddRef(component));

        child->conditionalAttachToStructure();
        m_childrenNames.insert(child->name());

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

        if (auto component = rtti_cast<SceneContentComponentNode>(child))
            m_components.remove(component);

        if (child->m_structure)
            child->conditionalDetachFromStructure();

        m_childrenNames.clear();

        handleChildRemoved(child); // call the notification while we still have a valid parent

        child->IObject::parent(nullptr);
        child->handleParentChanged();

        markModified();
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
            m_visualFlags.clear(flag);
            changed = true;
        }

        if (changed)
        {
            SceneContentNodePtr selfRef(AddRef(this));

            if (m_structure)
                m_structure->postEvent(EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED, selfRef);

            postEvent(EVENT_CONTENT_NODE_VISUAL_FLAG_CHANGED, selfRef);

            if (flag == SceneContentNodeVisualBit::SelectedNode)
            {
                if (type() == SceneContentNodeType::Entity)
                    markDirty(SceneContentNodeDirtyBit::Selection);
                else if (type() == SceneContentNodeType::Component)
                    if (auto entity = parent())
                        entity->markDirty(SceneContentNodeDirtyBit::Selection);
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
   


    void SceneContentNode::handleVisibilityChanged()
    {
        // TODO
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

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldLayer);
    RTTI_END_TYPE();

    SceneContentWorldLayer::SceneContentWorldLayer(const StringBuf& name)
        : SceneContentNode(SceneContentNodeType::LayerFile, name)
    {}

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentWorldDir);
    RTTI_END_TYPE();

    SceneContentWorldDir::SceneContentWorldDir(const StringBuf& name)
        : SceneContentNode(SceneContentNodeType::LayerDir, name)
    {}

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentPrefabRoot);
    RTTI_END_TYPE();

    SceneContentPrefabRoot::SceneContentPrefabRoot()
        : SceneContentNode(SceneContentNodeType::PrefabRoot, StringBuf("Prefab"))
    {} 

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentDataNode);
    RTTI_END_TYPE();

    SceneContentDataNode::SceneContentDataNode(SceneContentNodeType nodeType, const StringBuf& name, const EulerTransform& localToParent, const ObjectTemplatePtr& editableData, const ObjectTemplatePtr& baseData /*= nullptr*/)
        : SceneContentNode(nodeType, name)
        , m_localToParent(localToParent)
        , m_baseData(baseData)
        , m_editableData(editableData)
        , m_dataEvents(this)
    {
        if (m_baseData)
        {
            DEBUG_CHECK(m_baseData->parent() == nullptr);
            m_baseData->parent(this);
        }

        if (m_editableData)
        {
            DEBUG_CHECK(m_editableData->parent() == nullptr);
            m_editableData->parent(this);

            m_dataEvents.bind(m_editableData->eventKey(), EVENT_OBJECT_PROPERTY_CHANGED) = [this](StringBuf path)
            {
                handleDataPropertyChanged(path);
            };
        }

        cacheTransformData();
    }

    void SceneContentDataNode::displayText(IFormatStream& txt) const
    {
        TBaseClass::displayText(txt);

        if (m_baseData)
            txt << " [tag:#888]Overridden[/tag]";

        if (!m_editableData)
        {
            txt << " [tag:#888]No data[/tag]";
        }
        else if (m_editableData->cls() != base::world::EntityTemplate::GetStaticClass())
        {
            auto className = m_editableData->cls()->name().view();
            className = className.afterLastOrFull("::");
            className = className.beforeFirstOrFull("Template");
            txt.appendf(" [i]({})[/i]", className);
        }
    }

    void SceneContentDataNode::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        // TEMPSHIT
        rendering::scene::DebugLineDrawer dd(frame.geometry.solid);
        dd.axes(cachedLocalToWorldMatrix(), 0.1f);
    }

    void SceneContentDataNode::handleParentChanged()
    {
        cacheTransformData();
    }

    void SceneContentDataNode::handleDataPropertyChanged(const StringBuf& path)
    {
        
    }

    void SceneContentDataNode::changeData(const ObjectTemplatePtr& data)
    {
        if (m_editableData != data)
        {
            if (m_editableData)
            {
                DEBUG_CHECK(m_editableData->parent() == this);
                m_editableData->parent(nullptr);
                m_dataEvents.unbind(m_editableData->eventKey());
            }

            m_editableData = data;

            if (m_editableData)
            {
                DEBUG_CHECK(m_editableData->parent() == nullptr);
                m_editableData->parent(this);

                m_dataEvents.bind(m_editableData->eventKey(), EVENT_OBJECT_PROPERTY_CHANGED) = [this](StringBuf path)
                {
                    handleDataPropertyChanged(path);
                };
            }

            markDirty(SceneContentNodeDirtyBit::Content);

            {
                SceneContentNodePtr selfPtr(AddRef(static_cast<SceneContentNode*>(this)));

                postEvent(EVENT_CONTENT_NODE_RENAMED, selfPtr);

                if (structure())
                    structure()->postEvent(EVENT_CONTENT_STRUCTURE_NODE_RENAMED, selfPtr);
            }
        }
    }

    void SceneContentDataNode::changeLocalPlacement(const EulerTransform& newTramsform, bool force /*= false*/)
    {
        if (m_localToParent != newTramsform || force)
        {
            m_localToParent = newTramsform;
            updateTransform(force);
        }
    }

    void SceneContentDataNode::updateTransform(bool force /*= false*/, bool recursive /*= true*/)
    {
        if (cacheTransformData() || force)
        {
            if (recursive)
            {
                for (const auto& child : children())
                    if (auto* dataChild = rtti_cast<SceneContentDataNode>(child.get()))
                        dataChild->updateTransform(force, recursive);
            }
        }
    }

    bool SceneContentDataNode::cacheTransformData()
    {
        const auto prev = m_cachedLocalToWorldTransform;

        if (auto parentEntity = rtti_cast<SceneContentDataNode>(parent()))
            m_cachedLocalToWorldTransform = parentEntity->cachedLocalToWorldTransform() * m_localToParent.toTransform();
        else
            m_cachedLocalToWorldTransform = AbsoluteTransform::ROOT() * m_localToParent.toTransform();

        m_cachedLocalToWorldMatrix = m_cachedLocalToWorldTransform.approximate();

        return prev != m_cachedLocalToWorldTransform;
    }

    void SceneContentDataNode::handleTransformUpdated()
    {
        markDirty(SceneContentNodeDirtyBit::Transform);
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentEntityNodePrefabSource);
    RTTI_END_TYPE();

    SceneContentEntityNodePrefabSource::SceneContentEntityNodePrefabSource(const base::world::PrefabRef& prefab, bool enabled, bool inherited)
        : m_prefab(prefab)
        , m_enabled(enabled)
        , m_inherited(inherited)
    {}

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentEntityNode);
    RTTI_END_TYPE();

    SceneContentEntityNode::SceneContentEntityNode(const StringBuf& name, Array<RefPtr<SceneContentEntityNodePrefabSource>>&& rootPrefabs, const base::world::EntityTemplatePtr& editableData, const base::world::EntityTemplatePtr& baseData)
        : SceneContentDataNode(SceneContentNodeType::Entity, name, editableData->placement(), editableData, baseData)
        , m_prefabAssets(std::move(rootPrefabs))
    {
    }

    void SceneContentEntityNode::handleChildAdded(SceneContentNode* child)
    {
        TBaseClass::handleChildAdded(child);

        if (child->type() == SceneContentNodeType::Component)
            markDirty(SceneContentNodeDirtyBit::Content);
    }

    void SceneContentEntityNode::handleChildRemoved(SceneContentNode* child)
    {
        TBaseClass::handleChildRemoved(child);

        if (child->type() == SceneContentNodeType::Component)
            markDirty(SceneContentNodeDirtyBit::Content);
    }

    base::world::EntityTemplatePtr SceneContentEntityNode::compileData() const
    {
        if (auto cur = rtti_cast<base::world::EntityTemplate>(editableData()))
        {
            auto ret = CloneObject(cur);
            ret->rebase(baseData());
            ret->detach();
            ret->placement(cur->placement());
            return ret;
        }

        return nullptr;
    }

    EulerTransform MakeTransformRelative(const EulerTransform& cur, const EulerTransform& base)
    {
        EulerTransform ret = cur;
        ret.T -= base.T;
        ret.R -= base.R;
        ret.S /= base.S;
        return ret;
    }

    template< typename T >
    static bool IsDataNeeded(const T& data)
    {
        if (!data.placement().isIdentity())
            return true;

        return data.hasAnyOverrides();
    }

    void SceneContentEntityNode::handleDataPropertyChanged(const StringBuf& data)
    {
        markDirty(SceneContentNodeDirtyBit::Content);
    }

    base::world::NodeTemplatePtr SceneContentEntityNode::compileSnapshot() const
    {
        auto ret = CreateSharedPtr<world::NodeTemplate>();
        ret->m_name = StringID(name());

        // clone entity template data
        if (auto entityData = compileData())
        {
            entityData->parent(ret);
            ret->m_entityTemplate = entityData;
        }

        // capture component templates
        for (const auto& componentNode : components())
        {
            if (!componentNode->name().empty())
            {
                bool componentHasData = false;
                if (const auto componentData = componentNode->compileData())
                {
                    auto& entry = ret->m_componentTemplates.emplaceBack();
                    entry.name = StringID(componentNode->name());
                    entry.data = componentData;
                    componentData->parent(ret);
                }
            }
        }

        return ret;
    }

    world::NodeTemplatePtr SceneContentEntityNode::compileDifferentialData(bool& outAnyMeaningfulData) const
    {
        auto ret = CreateSharedPtr<world::NodeTemplate>();
        ret->m_name = StringID(name());

        // clone entity template data
        if (auto entityData = rtti_cast<base::world::EntityTemplate>(editableData()))
        {
            auto clonedEntityData = CloneObject(entityData);

            if (auto base = rtti_cast<base::world::EntityTemplate>(baseData()))
            {
                clonedEntityData->placement(MakeTransformRelative(clonedEntityData->placement(), base->placement()));
                outAnyMeaningfulData |= IsDataNeeded(*clonedEntityData);
            }
            else
            {
                clonedEntityData->placement(entityData->placement());
                outAnyMeaningfulData = true;
            }

            clonedEntityData->parent(ret);
            ret->m_entityTemplate = clonedEntityData;
        }

        // capture component templates
        for (const auto& componentNode : components())
        {
            if (!componentNode->name().empty())
            {
                bool componentHasData = false;
                if (const auto componentData = componentNode->compileDifferentialData(componentHasData))
                {
                    outAnyMeaningfulData |= componentHasData;

                    auto& entry = ret->m_componentTemplates.emplaceBack();
                    entry.name = StringID(componentNode->name());
                    entry.data = componentData;

                    componentData->parent(ret);
                }
            }
        }

        // capture children
        for (const auto& childEntity : entities())
        {
            bool entityHasData = false;
            if (const auto childEntityData = childEntity->compileDifferentialData(entityHasData))
            {
                outAnyMeaningfulData |= entityHasData;
                ret->m_children.pushBack(childEntityData);
            }
        }
        
        return ret;
    }    

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentComponentNode);
    RTTI_END_TYPE();

    SceneContentComponentNode::SceneContentComponentNode(const StringBuf& name, const base::world::ComponentTemplatePtr& editableData, const base::world::ComponentTemplatePtr& baseData)
        : SceneContentDataNode(SceneContentNodeType::Component, name, editableData->placement(), editableData, baseData)
    {
    }

    base::world::ComponentTemplatePtr SceneContentComponentNode::compileData() const
    {
        if (auto cur = rtti_cast<base::world::ComponentTemplate>(editableData()))
        {
            auto ret = CloneObject(cur);
            ret->rebase(baseData());
            ret->detach();
            ret->placement(cur->placement());
            return ret;
        }

        return nullptr;
    }

    base::world::ComponentTemplatePtr SceneContentComponentNode::compileDifferentialData(bool& outAnyMeaningfulData) const
    {
        auto componentData = rtti_cast<base::world::ComponentTemplate>(editableData());
        DEBUG_CHECK_RETURN_V(componentData, nullptr);

        auto ret = CloneObject(componentData);;

        if (auto base = rtti_cast<base::world::ComponentTemplate>(baseData()))
        {
            ret->placement(MakeTransformRelative(componentData->placement(), base->placement()));
            outAnyMeaningfulData |= IsDataNeeded(*ret);
        }
        else
        {
            ret->placement(componentData->placement());
            outAnyMeaningfulData = true;
        }

        return ret;
    }

    void SceneContentComponentNode::handleDataPropertyChanged(const StringBuf& data)
    {
        if (auto entity = rtti_cast<SceneContentEntityNode>(parent()))
            entity->markDirty(SceneContentNodeDirtyBit::Content);
    }

    //---

    static const int MAX_DEPTH = 10;


    template< typename T >
    static Transform MergeTransform(const Array<const T*>& templates)
    {
        Transform ret;
        bool first = true;

        for (auto i : templates.indexRange().reversed())
        {
            const auto* data = templates[i];
            if (!data->placement().isIdentity())
            {
                if (first)
                    ret = data->placement().toTransform();
                else
                    ret = data->placement().toTransform().applyTo(ret);
            }
        }

        return ret;
    }

    template< typename T >
    static RefPtr<T> MergeTemplates(const Array<const T*>& templates)
    {
        if (templates.size() == 0)
            return nullptr;

        if (templates.size() == 1)
            return AddRef(templates[0]);

        RefPtr<T> ret = nullptr;
        for (auto i : templates.indexRange().reversed())
        {
            auto copy = CloneObject(templates[i]);
            copy->rebase(ret);
            ret = copy;
        }

        return ret;
    }

    typedef HashMap<StringID, InplaceArray<const base::world::ComponentTemplate*, 4>>ComponentTemplateList;

    static void CollectComponentTemplates(const Array<const ::world::NodeTemplate*>& templates, ComponentTemplateList& outTemplates)
    {
        for (const auto* dataTemplate : templates)
            for (const auto& compTemplate : dataTemplate->m_componentTemplates)
                if (compTemplate.name && compTemplate.data && compTemplate.data->enabled())
                    outTemplates[compTemplate.name].pushBack(compTemplate.data);
    }

    static bool TransformHasTranslationOnly(const base::EulerTransform& cur)
    {
        return cur.R == base::EulerTransform::Rotation::ZERO() &&
            cur.S == base::EulerTransform::Scale::ONE();
    }

    static base::EulerTransform MergeTransforms(const base::EulerTransform& base, const base::EulerTransform& cur)
    {
        // handle most common case - no transform
        if (base.isIdentity())
            return cur;
        else if (cur.isIdentity())
            return base;

        base::EulerTransform ret;
        ret.T = base.T + cur.T;
        ret.R = base.R + cur.R;
        ret.S = base.S * cur.S;
        return ret;
    }

    template< typename T >
    static bool SplitAndMergeTemplates(const base::Array<const T*>& sourceTemplates, const base::world::NodeTemplate* rootNode, RefPtr<T>& outBaseData, RefPtr<T>& outEditableData)
    {
        RefPtr<T> baseData;

        for (const auto* data : sourceTemplates)
        {
            auto dataCopy = CloneObject<T>(data);

            const auto isEditableData = data->hasParent(rootNode); // is this data from source hierarchy
            if (isEditableData)
            {
                DEBUG_CHECK(!outEditableData); // duplicated editable data ?
                outEditableData = dataCopy;
            }
            else
            {
                if (baseData)
                {
                    const auto mergedTransform = MergeTransforms(baseData->placement(), dataCopy->placement());
                    dataCopy->rebase(baseData);
                    dataCopy->placement(mergedTransform);
                }

                baseData = dataCopy;
            }
        }

        // no data
        if (!outEditableData && !outBaseData)
            return false;

        // create a default object of matching class
        if (!outEditableData)
        {
            outEditableData = outBaseData->cls()->create<T>();
            outEditableData->rebase(outBaseData);
            outEditableData->placement(outBaseData->placement());
        }
        else if (outBaseData)
        {
            const auto mergedTransform = MergeTransforms(outBaseData->placement(), outEditableData->placement());
            outEditableData->rebase(outBaseData);
            outEditableData->placement(mergedTransform);
        }

        return true;
    }

    static SceneContentEntityNodePtr CompileEntityContent(const base::world::NodeTemplate* rootNode, const StringBuf& name, const Array<const base::world::NodeTemplate*>& templates)
    {
        PC_SCOPE_LVL1(CreateEntity);

        // collect valid entity templates to build an entity from
        InplaceArray<const base::world::EntityTemplate*, 8> entityTemplates;
        for (const auto& dataTemplate : templates)
        {
            const auto& entityData = dataTemplate->m_entityTemplate;
            if (entityData && entityData->enabled())
                entityTemplates.pushBack(entityData);
        }

        // generate base and editable entity data
        base::world::EntityTemplatePtr baseEntityData, editableEntityData;
        if (!SplitAndMergeTemplates(entityTemplates, rootNode, baseEntityData, editableEntityData))
            return false;
        DEBUG_CHECK(editableEntityData);

        // prefabs
        Array<RefPtr<SceneContentEntityNodePrefabSource>> prefabs;

        // create the content node
        auto entityNode = base::CreateSharedPtr<SceneContentEntityNode>(name, std::move(prefabs), editableEntityData, baseEntityData);

        // gather list of all components to create
        ComponentTemplateList namedComponentTemplates;
        CollectComponentTemplates(templates, namedComponentTemplates);

        // create all named components and attach them to entity
        namedComponentTemplates.forEach([&entityNode, rootNode](StringID name, const Array<const base::world::ComponentTemplate*>& templates)
            {
                // extract data
                base::world::ComponentTemplatePtr baseComponentData, editableComponentData;
                if (SplitAndMergeTemplates(templates, rootNode, baseComponentData, editableComponentData))
                {
                    // create editable node
                    auto componentNode = base::CreateSharedPtr<SceneContentComponentNode>(StringBuf(name.view()), editableComponentData, baseComponentData);
                    entityNode->attachChildNode(componentNode);
                }
            });

        return entityNode;
    }

    SceneContentEntityNodePtr ProcessSingleEntity(int depth, const base::world::NodeTemplate* rootNode, const StringBuf& name, const base::world::NodeCompilationStack& it, Array<SceneContentEntityNodePtr>* outAllEntities)
    {
        InplaceArray<base::world::PrefabRef, 10> prefabsToInstance;
        it.collectPrefabs(prefabsToInstance);

        // "instance" prefabs
        base::world::NodeCompilationStack localIt(it);
        for (auto& prefab : prefabsToInstance)
        {
            if (auto data = prefab.acquire())
            {
                const auto rootIndex = 0;
                if (rootIndex >= 0 && rootIndex <= data->nodes().lastValidIndex())
                {
                    if (const auto rootNode = data->nodes()[rootIndex])
                        localIt.pushBack(rootNode);
                }
            }
        }

        // compile single entity out of the current stack
        if (auto compiledNode = CompileEntityContent(rootNode, name, localIt.templates()))
        {
            if (outAllEntities)
                outAllEntities->pushBack(compiledNode);

            // create child entities
            if (depth < MAX_DEPTH)
            {
                HashSet<StringID> childrenNames;
                localIt.collectChildNodeNames(childrenNames);
                for (const auto name : childrenNames)
                {
                    base::world::NodeCompilationStack childIt;
                    localIt.enterChild(name, childIt);

                    ProcessSingleEntity(depth + 1, rootNode, StringBuf(name.view()), childIt, outAllEntities);
                }
            }

            return compiledNode;
        }
        else
        {
            return nullptr;
        }
    }

    SceneContentEntityNodePtr UnpackNode(const base::world::NodeTemplate* rootNode, Array<SceneContentEntityNodePtr>* outAllEntities /*= nullptr*/)
    {
        base::world::NodeCompilationStack stack;
        stack.pushBack(rootNode);

        return ProcessSingleEntity(0, rootNode, StringBuf(rootNode->m_name.view()), stack, outAllEntities);
    }

    //---

} // ed