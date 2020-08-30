/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: node #]
***/

#include "build.h"
#include "editorNode.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_CLASS(EditorNodeTransform);
        RTTI_PROPERTY(m_position);
        RTTI_PROPERTY(m_rotation);
        RTTI_PROPERTY(m_scale);
    RTTI_END_TYPE();

    EditorNodeTransform::EditorNodeTransform()
        : m_scale(1, 1, 1)
    {
        m_flags = { EditorNodeTransformFlagBit::Identity, EditorNodeTransformFlagBit::LocalToWorldCached, EditorNodeTransformFlagBit::WorldToLocalCached };
        m_localToWorld.identity();
        m_worldToLocal.identity();
    }

    EditorNodeTransform::~EditorNodeTransform()
    {}

    AbsoluteTransform EditorNodeTransform::toTransform() const
    {
        return AbsoluteTransform(m_position, m_rotation.toQuat(), m_scale);
    }

    const Matrix& EditorNodeTransform::evalLocalToWorld()
    {
        if (!m_flags.test(EditorNodeTransformFlagBit::LocalToWorldCached))
        {
            m_localToWorld = toTransform().approximate();
            m_flags |= EditorNodeTransformFlagBit::LocalToWorldCached;
        }

        return m_localToWorld;
    }

    const Matrix& EditorNodeTransform::evalWorldToLocal()
    {
        if (!m_flags.test(EditorNodeTransformFlagBit::WorldToLocalCached))
        {
            m_worldToLocal = evalLocalToWorld().inverted();
            m_flags |= EditorNodeTransformFlagBit::WorldToLocalCached;
        }

        return m_worldToLocal;
    }

    void EditorNodeTransform::onPostLoad()
    {
        TBaseClass::onPostLoad();
        invalidateMatrices();
    }

    void EditorNodeTransform::setup(const AbsolutePosition& position, const Angles& rotation /*= Angles::ZERO()*/, const Vector3& scale /*= Vector3::ONE()*/)
    {
        m_position = position;
        m_rotation = rotation;
        m_scale = scale;
        invalidateMatrices();
    }

    void EditorNodeTransform::position(const AbsolutePosition& position)
    {
        if (m_position != position)
        {
            m_position = position;
            invalidateMatrices();
        }
    }

    void EditorNodeTransform::rotation(const Angles& rotation)
    {
        if (m_rotation != rotation)
        {
            m_rotation = rotation;
            invalidateMatrices();
        }
    }

    void EditorNodeTransform::scale(const Vector3& scale)
    {
        if (m_scale != scale)
        {
            m_scale = scale;
            invalidateMatrices();
        }
    }

    void EditorNodeTransform::invalidateMatrices()
    {
        m_flags.clearAll();

        for (;;)
        {
            if (m_position.approximate() != Vector3::ZERO())
                break;
            if (m_rotation != Angles::ZERO())
                break;
            if (m_scale != Vector3::ONE())
                break;

            m_flags |= EditorNodeTransformFlagBit::Identity;
        }

        if (m_scale != Vector3::ONE())
        {
            m_flags |= EditorNodeTransformFlagBit::HasScale;

            bool flip = false;
            if (m_scale.x < 0.0f) flip = !flip;
            if (m_scale.y < 0.0f) flip = !flip;
            if (m_scale.z < 0.0f) flip = !flip;

            if (flip)
                m_flags |= EditorNodeTransformFlagBit::FlipFaces;

            if (m_scale.x != m_scale.y || m_scale.y != m_scale.z || m_scale.x != m_scale.z)
                m_flags |= EditorNodeTransformFlagBit::HasNonUniformScale;
        }
    }

    //---

    RTTI_BEGIN_TYPE_CLASS(EditorNodePivot);
        RTTI_PROPERTY(m_position);
        RTTI_PROPERTY(m_rotation);
    RTTI_END_TYPE();

    EditorNodePivot::EditorNodePivot()
    {}

    EditorNodePivot::~EditorNodePivot()
    {}

    //---

    RTTI_BEGIN_TYPE_CLASS(EditorNode);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_children);
        RTTI_PROPERTY(m_transform);
        RTTI_PROPERTY(m_pivot);
    RTTI_END_TYPE();

    EditorNode::EditorNode()
        : m_selected(false)
        , m_visibleMerged(true)
        , m_visibleLocal(true)
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_transform = CreateSharedPtr<EditorNodeTransform>();
            m_transform->parent(this);

            m_pivot = CreateSharedPtr<EditorNodePivot>();
            m_pivot->parent(this);
        }
        EditorNodeWeakPtr m_parent;

        EditorNodeTransformPtr m_transform;
        EditorNodePivotPtr m_pivot;
        EditorNodeTransformFlags m_flags;
        Array<EditorNodePtr> m_children;

        StringBuf m_name;
    }

    EditorNode::~EditorNode()
    {}

    void EditorNode::attachChild(EditorNode* node)
    {
        DEBUG_CHECK_RETURN(m_children.contains(node));
        DEBUG_CHECK_RETURN(node != nullptr);
        DEBUG_CHECK_RETURN(node->parent() == nullptr);

        auto nodeRef = EditorNodePtr(AddRef(node));
        node->IObject::parent(this);
        m_children.pushBack(nodeRef);

        node->m_visibleMerged = m_visibleMerged & node->m_visibleLocal;

        if (m_scene)
            node->attachScene(m_scene);

        DispatchGlobalEvent(eventKey(), EVENT_EDITOR_CHILD_NODE_ATTACHED, nodeRef);
    }

    void EditorNode::detachChild(EditorNode* node)
    {
        DEBUG_CHECK_RETURN(m_children.contains(node));
        DEBUG_CHECK_RETURN(node != nullptr);
        DEBUG_CHECK_RETURN(node->parent() == this);

        auto nodeRef = EditorNodePtr(AddRef(node));
        DispatchGlobalEvent(eventKey(), EVENT_EDITOR_CHILD_NODE_DETACHED, nodeRef);

        if (m_scene)
            node->detachScene(m_scene);

        m_children.remove(node);
        node->IObject::parent(nullptr);
    }

    void EditorNode::attachScene(EditorScene* scene)
    {
        DEBUG_CHECK_RETURN(m_scene == nullptr);
        DEBUG_CHECK_RETURN(scene != nullptr);

        m_scene = scene;
        handleSceneAttached(scene);

        if (m_visibleMerged)
            handleCreateVisualization();

        for (const auto& child : m_children)
            child->attachScene(scene);
    }

    void EditorNode::detachScene(EditorScene* scene)
    {
        DEBUG_CHECK_RETURN(m_scene == scene);

        for (const auto& child : m_children)
            child->detachScene(scene);

        handleDestroyVisualization();

        handleSceneDetached(scene);
        m_scene = nullptr;
    }

    void EditorNode::update(float dt)
    {
        handleUpdate(dt);
    }

    void EditorNode::render(rendering::scene::FrameParams& params)
    {
        if (!m_visibleMerged)
            return;
    }

    void EditorNode::updateVisibility()
    {
        auto mergedVisible = m_visibleLocal;
        if (auto parentNode = parent())
            mergedVisible &= parentNode->mergedVisible();

        if (mergedVisible != m_visibleMerged)
        {
            if (!mergedVisible)
                handleDestroyVisualization();

            m_visibleMerged = mergedVisible;

            if (mergedVisible)
                handleCreateVisualization();

            for (const auto& child : m_children)
                child->updateVisibility();
        }
    }

    void EditorNode::visibility(bool visible)
    {
        if (m_visibleLocal != visible)
        {
            m_visibleLocal = visible;
            updateVisibility();
        }
    }

    //---

    DataViewPtr EditorNode::contentDataView()
    {
        return nullptr;
    }

    void EditorNode::handleChildAttached(EditorNode* child)
    {}

    void EditorNode::handleChildDetached(EditorNode* child)
    {}

    void EditorNode::handleSceneAttached(EditorScene* scene)
    {}

    void EditorNode::handleSceneDetached(EditorScene* scene)
    {}

    void EditorNode::handleCreateVisualization()
    {}

    void EditorNode::handleDestroyVisualization()
    {}

    void EditorNode::handleUpdateTransform()
    {}

    void EditorNode::handleUpdate(float dt)
    {}

    //--

} // ed