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

BEGIN_BOOMER_NAMESPACE()

//---

enum class EntityFlagBit : uint64_t
{
    Hidden = FLAG(0),
    TempHidden = FLAG(1),
    NoFadeIn = FLAG(2),
    NoFadeOut = FLAG(3),
    NoDetailChange = FLAG(4),
    Selected = FLAG(5),
    Attaching = FLAG(10),
    Detaching = FLAG(11),
    Attached = FLAG(12),
    DirtyTransform = FLAG(13),
    //CastShadows = FLAG(10),
    //ReceiveShadows = FLAG(11),
};

typedef DirectFlags<EntityFlagBit> EntityFlags;

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

    AbsolutePosition position;
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

// a basic game dweller, contains components and links between them
class ENGINE_WORLD_API Entity : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(Entity, IObject);

public:
    Entity();
    virtual ~Entity();

    //--

    /// get the world this entity is part of (NOTE: entity may NOT be yet attached)
    INLINE World* world() const { return m_world; }

    /// is the entity attached ?
    INLINE bool attached() const { return m_flags.test(EntityFlagBit::Attached); }

    /// is the entity selected ?
    INLINE bool selected() const { return m_flags.test(EntityFlagBit::Selected); }

    /// get absolute placement of the component
    /// NOTE: look at World::moveEntity() for a way to move entity to different place
    INLINE const AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

    /// current entity placement (updated ONLY in handleTransformUpdate)
    INLINE const Matrix& localToWorld() const { return m_localToWorld; }

    /// get selection owner ID (editor)
    INLINE const uint32_t selectionOwner() const { return m_selectionOwner; }

    //--

    /// request hierarchy of this entity to be updated
    void requestTransformUpdate();

    /// request entity to be moved to new location
    /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
    void requestTransform(const AbsoluteTransform& newTransform);

    /// request entity to be moved to new location keeping existing rotation/scale
    /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
    void requestMove(const AbsolutePosition& newPosition);

    /// request entity to be moved to new location/orientation but keeping existing scale
    /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
    void requestMove(const AbsolutePosition& newPosition, const Quat& newRotation);

    /// toggle selection flag
    void requestSelectionChange(bool flag);

    /// bind selection owner
    void bindSelectionOwner(uint32_t selectionOwner);

    //--

    /// update part before the systems are updated (good place to process input and send it to systems)
    virtual void handlePreTick(float dt);

    /// update part after the systems are updated (good place to suck data from systems)
    virtual void handlePostTick(float dt);

    /// update transform chain of this entity, NOTE: can be called even if we don't have world attached (it will basically dry-move entity to new place)
    virtual void handleTransformUpdate(const AbsoluteTransform& transform);

    /// handle attachment to world, called during world update
    virtual void handleAttach();

    /// handle detachment from scene
    virtual void handleDetach();

    /// render debug elements
    virtual void handleDebugRender(rendering::FrameParams& frame) const;

    /// handle selection flag change
    virtual void handleSelectionChanged();

    //--

    /// handle game input event
    virtual bool handleInput(const input::BaseEvent& evt);

    /// calculate viewport
    virtual bool handleCamera(CameraSetup& outCamera);

    /// handle stream-in of the component island, called after children were streamed in
    /// NOTE: none of the entities in the island are attached at the time of stream it so we can modify the hierarchy freely
    virtual void handleStreamIn(StreamingIslandInstance* island);

    /// handle stream-out of the component island, called after children were streamed out
    virtual void handleStreamOut(StreamingIslandInstance* island);

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

private:
    World* m_world = nullptr;
    EntityFlags m_flags;

    uint32_t m_selectionOwner = 0;

    //--

    AbsoluteTransform m_absoluteTransform; // absolute entity transform
    Matrix m_localToWorld; // computed during last transform update

    //--

    void attachToWorld(World* world);
    void detachFromWorld(World* world);

    //--

    IWorldSystem* systemPtr(short index) const;

    //--

    friend class World;
};

///---

END_BOOMER_NAMESPACE()
