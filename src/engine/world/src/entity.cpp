/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "world.h"
#include "entity.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(EntityInputEvent);
    RTTI_PROPERTY(m_name);
    RTTI_PROPERTY(m_deltaValue);
    RTTI_PROPERTY(m_absoluteValue);
RTTI_END_TYPE();

EntityInputEvent::EntityInputEvent()
{}

//--

RTTI_BEGIN_TYPE_CLASS(EntityCameraPlacement);
    RTTI_PROPERTY(position);
    RTTI_PROPERTY(rotation);
    RTTI_PROPERTY(customFov);
    RTTI_PROPERTY(customAspect);
    RTTI_PROPERTY(customNearPlane);
    RTTI_PROPERTY(customFarPlane);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(Entity);
    RTTI_PROPERTY(m_absoluteTransform);
RTTI_END_TYPE();

//--

IEntityHelperStreamingDistanceResolver::~IEntityHelperStreamingDistanceResolver()
{}

//--

Entity::Entity()
{}

Entity::~Entity()
{
    DEBUG_CHECK_EX(!m_world, "Destroying attached entity");
    DEBUG_CHECK_EX(!attached(), "Destroying attached entity");
}

Box Entity::calcBounds() const
{
    const auto center = absoluteTransform().position().approximate();
    return Box(center, 0.1f);
}

Box Entity::calcStreamingBounds(IEntityHelperStreamingDistanceResolver& distanceResolver) const
{
    const auto center = absoluteTransform().position().approximate();
    return Box(center, 1.0f);
}

void Entity::handlePreTick(float dt)
{
    // TODO: scripts :)
}

void Entity::handlePostTick(float dt)
{
    // TODO: scripts :)
}

void Entity::requestTransformUpdate()
{
    if (m_world)
    {
        if (!m_flags.test(EntityFlagBit::DirtyTransform))
        {
            m_flags |= EntityFlagBit::DirtyTransform;
            m_world->scheduleEntityForTransformUpdate(this);
        }
    }
    else
    {
        handleTransformUpdate(m_absoluteTransform);
    }
}

void Entity::requestTransform(const AbsoluteTransform& newTransform)
{
    if (m_world)
    {
        m_flags |= EntityFlagBit::DirtyTransform;
        m_world->scheduleEntityForTransformUpdateWithTransform(this, newTransform);
    }
    else
    {
        handleTransformUpdate(newTransform);
    }
}

void Entity::requestMove(const AbsolutePosition& newPosition)
{
    auto transform = m_absoluteTransform;
    transform.position(newPosition);
    requestTransform(transform);
}

void Entity::requestMove(const AbsolutePosition& newPosition, const Quat& newRotation)
{
    auto transform = m_absoluteTransform;
    transform.position(newPosition);
    transform.rotation(newRotation);
    requestTransform(transform);
}

void Entity::requestSelectionChange(bool flag)
{
    if (selected() != flag)
    {
        if (flag)
            m_flags |= EntityFlagBit::Selected;
        else
            m_flags -= EntityFlagBit::Selected;

        handleSelectionChanged();
    }
}

void Entity::bindSelectionOwner(uint32_t selectionOwner)
{
    m_selectionOwner = selectionOwner;
}

void Entity::handleSelectionChanged()
{
}

void Entity::handleTransformUpdate(const AbsoluteTransform& transform)
{
    // update entity transform
    m_absoluteTransform = transform;
    m_localToWorld = m_absoluteTransform.approximate();

    // transform is no longer invalid
    m_flags -= EntityFlagBit::DirtyTransform;

    // TODO: post transform to the entities attached via transform link
}

void Entity::handleAttach()
{
    ASSERT(m_flags.test(EntityFlagBit::Attaching));
}

void Entity::handleDetach()
{
    ASSERT(m_flags.test(EntityFlagBit::Detaching));
}

void Entity::handleDebugRender(rendering::FrameParams& frame) const
{

}

bool Entity::handleInput(const input::BaseEvent& evt)
{
    return false;
}

bool Entity::handleCamera(EntityCameraPlacement& outCamera)
{
    outCamera.position = absoluteTransform().position();
    outCamera.rotation = absoluteTransform().rotation();
    return true;
}

void Entity::handleStreamIn(StreamingIslandInstance* island)
{
    // nothing
}

void Entity::handleStreamOut(StreamingIslandInstance* island)
{
    // nothing
}

IWorldSystem* Entity::systemPtr(short index) const
{
    if (m_world)
        return m_world->systemPtr(index);
    return nullptr;
}

//--

void Entity::attachToWorld(World* world)
{
    DEBUG_CHECK_RETURN(world);
    DEBUG_CHECK_RETURN(!m_world);
    DEBUG_CHECK_RETURN(!m_flags.test(EntityFlagBit::Attached));
    DEBUG_CHECK_RETURN(!m_flags.test(EntityFlagBit::Attaching));
    DEBUG_CHECK_RETURN(!m_flags.test(EntityFlagBit::Detaching));

    m_world = world;
    m_flags |= EntityFlagBit::Attaching;
    handleAttach();
    m_flags -= EntityFlagBit::Attaching;
    m_flags |= EntityFlagBit::Attached;
}

void Entity::detachFromWorld(World* world)
{
    DEBUG_CHECK_RETURN(world);
    DEBUG_CHECK_RETURN(m_world == world);
    DEBUG_CHECK_RETURN(m_flags.test(EntityFlagBit::Attached));
    DEBUG_CHECK_RETURN(!m_flags.test(EntityFlagBit::Attaching));
    DEBUG_CHECK_RETURN(!m_flags.test(EntityFlagBit::Detaching));

    m_flags |= EntityFlagBit::Detaching;
    handleDetach();
    m_flags -= EntityFlagBit::Detaching;
    m_flags -= EntityFlagBit::Attached;
    m_world = nullptr;
}

//--

SpecificClassType<Entity> Entity::determineEntityTemplateClass(const ITemplatePropertyValueContainer& templateProperties)
{
    return templateProperties.compileClass().cast<Entity>();
}

void Entity::queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const
{
    TBaseClass::queryTemplateProperties(outTemplateProperties);

    outTemplateProperties.prop("Streaming"_id, "streamingGroupChildren"_id, true, rtti::PropertyEditorData().comment("Stream all child entities together as a group (better for logic and visuals, bad for heavy prefabs)"));
    outTemplateProperties.prop("Streaming"_id, "streamingBreakFromGroup"_id, false, rtti::PropertyEditorData().comment("Force this entity to stream individually from it's parent (parent still has to be streamed first, there's no override for this)"));
    outTemplateProperties.prop("Streaming"_id, "streamingDistanceOverride"_id, 0.0f, rtti::PropertyEditorData().comment("Override distance for the streaming range"));

    outTemplateProperties.prop("Transform"_id, "attachToParentEntity"_id, false, rtti::PropertyEditorData().comment("In game follow parent entity"));
}

bool Entity::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
{
    if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
        return false;

    return true;
}

//--

END_BOOMER_NAMESPACE()
