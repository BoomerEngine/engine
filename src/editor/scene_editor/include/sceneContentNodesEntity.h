/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

#include "sceneContentNodes.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// general node with editable content (component/entity)
class EDITOR_SCENE_EDITOR_API SceneContentDataNode : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentDataNode, SceneContentNode);

public:
    SceneContentDataNode(SceneContentNodeType nodeType, const StringBuf& name, const ObjectIndirectTemplate* editableData);

    // do we have override content defined at this node (coming from prefab)
    INLINE bool hasBaseContent() const { return !m_baseData.empty(); }

    // base data of this node, always valid if we come from prefab - NOTE: we might have more than one base node (if so, data is stacked)
    INLINE const Array<ObjectIndirectTemplatePtr>& baseData() const { return m_baseData; }

    // editable data of this node, always valid, stacked on top of any base nodes
    INLINE const ObjectIndirectTemplatePtr& editableData() const { return m_editableData; }

    // get current local to parent transform (it's always the placement from editable data)
    INLINE const EulerTransform& localToParent() const { return m_localToWorld; }

    // get the cached "local to world" for this node
    INLINE const Transform& cachedLocalToWorldTransform() const { return m_cachedLocalToWorldPlacement; }

    // get the cached "local to world" for this node
    INLINE const Matrix& cachedLocalToWorldMatrix() const { return m_cachedLocalToWorldMatrix; }

    //--

    // get parent transform
    const Transform& cachedParentToWorldTransform() const;

    // change local to parent placement
    // NOTE: placement of child nodes is NOT changed
    void changeLocalPlacement(const EulerTransform& localToParent, bool force = false);

    // change class of data
    void changeClass(ClassType templateClass);

    // determine data class
    ClassType dataClass() const;

    // flatten template data
    ObjectIndirectTemplatePtr compileFlatData() const;

    //--

protected:
    void cacheTransformData();
    void updateBaseTemplates(const Array<const ObjectIndirectTemplate*>& baseTemplates);

    virtual void displayTags(IFormatStream& txt) const override;

    virtual void handleTransformUpdated();
    virtual void handleDebugRender(rendering::FrameParams& frame) const override;
    virtual void handleParentChanged() override;
    virtual void handleDataPropertyChanged(const StringBuf& data);

private:
    Transform m_cachedLocalToWorldPlacement; // local to world placement for the node
    Matrix m_cachedLocalToWorldMatrix; // changed whenever parent changes as well

    ObjectIndirectTemplatePtr m_editableData; // always valid
    InplaceArray<ObjectIndirectTemplatePtr, 2> m_baseData; // valid only if we are instanced from a prefab
    EulerTransform m_baseLocalToTransform;

    EulerTransform m_localToWorld; // editable + base

    GlobalEventTable m_dataEvents;
};

//--

struct SceneContentEntityInstancedContent
{
    Array<RawEntityPrefabSetup> localPrefabs;
    Array<SceneContentNodePtr> childNodes;
};

/// general entity node (always in a file, can be parented to other entity)
class EDITOR_SCENE_EDITOR_API SceneContentEntityNode : public SceneContentDataNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentEntityNode, SceneContentDataNode);

public:
    SceneContentEntityNode(const StringBuf& name, const RawEntityPtr& node, const Array<RawEntityPtr>& inheritedTemplated = Array<RawEntityPtr>());

    INLINE const Array<RawEntityPrefabSetup>& localPrefabs() const { return m_localPrefabs; }

    RawEntityPtr compileDifferentialData() const;

    RawEntityPtr compileSnapshot() const;

    RawEntityPtr compiledForCopy() const;

    void invalidateData();

    //--

    void extractCurrentInstancedContent(SceneContentEntityInstancedContent& outInstancedContent); // NOTE: destructive !
    void createInstancedContent(const RawEntity* dataTemplate, SceneContentEntityInstancedContent& outInstancedContent) const;
    void applyInstancedContent(const SceneContentEntityInstancedContent& content);

    //--

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;

    bool canExplodePrefab() const;

private:
    Array<RawEntityPtr> m_inheritedTemplates;
    Array<RawEntityPrefabSetup> m_localPrefabs;

    bool m_dirtyVisualData = false;
    bool m_dirtyVisualTransform = false;

    virtual void handleDataPropertyChanged(const StringBuf& data) override;
    virtual void handleChildAdded(SceneContentNode* child) override;
    virtual void handleChildRemoved(SceneContentNode* child) override;
    virtual void handleVisibilityChanged() override;

    void updateBaseTemplates();
    void collectBaseNodes(const Array<RawEntityPrefabSetup>& localPrefabs, Array<const RawEntity*>& outBaseNodes) const;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
