/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

#include "core/object/include/object.h"
#include "core/object/include/objectSelection.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// generic input event
class ENGINE_WORLD_API EntityInputEvent
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_NONVIRTUAL_CLASS(EntityInputEvent);

public:
    StringID m_name;
    float m_deltaValue = 0.0f;
    float m_absoluteValue = 0.0f;

    EntityInputEvent();
};

//---

/// generic camera information
struct ENGINE_WORLD_API EntityCameraPlacement
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(EntityCameraPlacement);

    ExactPosition position;
    Quat rotation;

    float customFov = 75.0f;
    float customAspect = 0.0f;
    float customNearPlane = 0.0f;
    float customFarPlane = 0.0f;
};

//---

// helper class to lookup streaming distance for given resource
class ENGINE_WORLD_API IEntityHelperStreamingDistanceResolver : public NoCopy
{
public:
    virtual ~IEntityHelperStreamingDistanceResolver();

    /// fetch streaming distance for given resource
    virtual bool queryResourceStreamingDistance(const ResourceID& id, float& outDistance) = 0;
};

//---

// entity lock token, always passed to all threaded functions
class ENGINE_WORLD_API EntityThreadContext : public NoCopy
{
public:
    EntityThreadContext();

    uint32_t m_id = 0; // tempshit
};

//---

// entity editor state, accessible only for entities while they are in the editor
struct ENGINE_WORLD_API EntityEditorState
{
    Selectable selectable;

    bool selected = false;

    EntityEditorState();
    EntityEditorState(const EntityEditorState& other);
    EntityEditorState& operator=(const EntityEditorState& other);
};

//---

// a basic game dweller, contains components and links between them
class ENGINE_WORLD_API Entity : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(Entity, IObject);

public:
    Entity();
    virtual ~Entity();

    //--

    /// get assigned static ID, valid only for cooked entities (usually from the editor)
    INLINE EntityStaticID staticId() const { return m_staticId; }

    /// entity runtime ID, globally unique and not repeating
    INLINE EntityRuntimeID runtimeId() const { return m_runtimeId; }

    /// editor given name, there are no uniqueness checks at runtime
    INLINE StringID name() const { return m_name; }

    /// get the world this entity is part of (NOTE: entity may NOT be yet attached)
    INLINE World* world() const { return m_world; }

    /// is the entity attached ?
    INLINE bool attached() const { return m_world != nullptr; }

    /// entity's transform (in parent's space)
    INLINE const Transform& transform() const { return m_transform; }

    /// cached world space transform 
    INLINE const Transform& cachedWorldTransform() const { return m_cachedLocalToWorldTransfrom; }

    /// cached world space matrix
    INLINE const Matrix& cachedLocalToWorldMatrix() const { return m_cachedLocalToWorldMatrix; }

    // get editor state
    INLINE const EntityEditorState& editorState() const { return m_editorState; }

    //--

    /// get current transform parent
    EntityPtr transformParent() const;

    /// request change of local transform, only position is changed
    void requestTransformChange(const ExactPosition& pos);

    /// request change of local transform, only position and rotation is changed
    void requestTransformChange(const ExactPosition& pos, const Quat& rot);

    /// request change of local transform, all components are changed
    void requestTransformChange(const ExactPosition& pos, const Quat& rot, const Vector3& scale);

    /// request change of local transform, all components are changed
    void requestTransformChange(const Transform& transform);

    /// request change of transform to match given absolute world space transform
    void requestTransformChangeWorldSpace(const Transform& transform);

    // request change in transform parent
    void requestTransformParentChange(Entity* newEntityToFollow, bool keepWorldPosition);

    /// transform of this entity was updated
    virtual void handleTransformUpdate(const EntityThreadContext& tc);

    //--

     /// update parent of this node
    EntityPtr updateParent() const;

    /// entity update
    virtual void handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt);

    /// ask entity what kind of update it needs
    virtual void handleUpdateMask(WorldUpdateMask& outUpdateMask) const;

    //--

    /// handle attachment to world, called during world update
    virtual void handleAttach();

    /// handle detachment from scene
    virtual void handleDetach();

    //--

    /// render debug elements
    virtual void handleDebugRender(rendering::FrameParams& frame) const;

    /// handle game input event
    virtual bool handleInput(const InputEvent& evt);

    /// handle stream-in of the component island, called after children were streamed in
    /// NOTE: none of the entities in the island are attached at the time of stream it so we can modify the hierarchy freely
    virtual void handleStreamIn(CompiledStreamingIslandInstance* island);

    /// handle stream-out of the component island, called after children were streamed out
    virtual void handleStreamOut(CompiledStreamingIslandInstance* island);

    //--

    /// get a world system, returns NULL if entity or is not attached
    template< typename T >
    INLINE T* system()
    {
        static auto userIndex = ClassID<T>()->userIndex();
        return (T*)systemPtr(userIndex);
    }

    //---

    /// calculate entity bounds using visible components
    virtual Box calcBounds() const;

    /// calculate streaming distance of the entity
    virtual Box calcStreamingBounds(IEntityHelperStreamingDistanceResolver& distanceResolver) const;

    //---

    // determine final entity class
    virtual SpecificClassType<Entity> determineEntityTemplateClass(const ITemplatePropertyValueContainer& templateProperties);

    // list template properties for the entity
    virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const override;

    // initialize entity from template properties
    virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties) override;

    //---

    // toggle editor state
    virtual void handleEditorStateChange(const EntityEditorState& state);

private:
    World* m_world = nullptr;

    EntityStaticID m_staticId = 0; // valid only when entity was part of cooked content, assumed to be globally unique identifier of the entity
    EntityRuntimeID m_runtimeId = 0; // never repeating numerical ID

    StringID m_name; // assigned when attaching to world

    Transform m_transform;

    EntityEditorState m_editorState;

    //--

    Entity* m_transformParent = nullptr;
    Entity* m_updateParent = nullptr;

    Transform m_cachedLocalToWorldTransfrom;
    Matrix m_cachedLocalToWorldMatrix;

    //--

    void attachToWorld(World* world, const Transform& localTransform, const Transform& worldTransform, StringID name, EntityRuntimeID runtimeID, EntityStaticID staticID);
    void detachFromWorld(World* world);
    void applyTransform(World* world, const EntityThreadContext& tc, const Transform& localTransform, const Transform& worldTransform);

    //--

    IWorldSystem* systemPtr(short index) const;

    //--

    friend class World;
    friend class EntityThreadLock;
};

///---

END_BOOMER_NAMESPACE()
