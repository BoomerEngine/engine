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
#include "worldEntity.h"

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

EntityThreadContext::EntityThreadContext()
{}

//--

EntityEditorState::EntityEditorState() = default;
EntityEditorState::EntityEditorState(const EntityEditorState& other) = default;
EntityEditorState& EntityEditorState::operator=(const EntityEditorState& other) = default;

//--

RTTI_BEGIN_TYPE_CLASS(Entity);
    RTTI_PROPERTY(m_transform);
RTTI_END_TYPE();

//--

IEntityHelperStreamingDistanceResolver::~IEntityHelperStreamingDistanceResolver()
{}

//--

Entity::Entity()
{
}

Entity::~Entity()
{
    DEBUG_CHECK_EX(!m_world, "Destroying attached entity");
    DEBUG_CHECK_EX(!attached(), "Destroying attached entity");
}

//--

Box Entity::calcBounds() const
{
    return Box(m_cachedLocalToWorldTransfrom.T, 0.1f);
}

Box Entity::calcStreamingBounds(IEntityHelperStreamingDistanceResolver& distanceResolver) const
{
    return Box(m_cachedLocalToWorldTransfrom.T, 1.0f);
}

//--

EntityPtr Entity::transformParent() const
{
    return nullptr;
}

void Entity::requestTransformChange(const ExactPosition& pos)
{
    auto transform = m_transform;
    transform.T = pos;
    requestTransformChange(transform);
}

void Entity::requestTransformChange(const ExactPosition& pos, const Quat& rot)
{
    auto transform = m_transform;
    transform.T = pos;
    transform.R = rot;
    requestTransformChange(transform);
}

void Entity::requestTransformChange(const ExactPosition& pos, const Quat& rot, const Vector3& scale)
{
    auto transform = m_transform;
    transform.T = pos;
    transform.R = rot;
    transform.S = scale;
    requestTransformChange(transform);
}

void Entity::requestTransformChange(const Transform& transform)
{
    if (m_world)
        m_world->scheduleEntityForTransformUpdate(this, true, transform);
    else
        m_transform = transform;
}

void Entity::requestTransformChangeWorldSpace(const Transform& transform)
{
    if (m_world)
        m_world->scheduleEntityForTransformUpdate(this, false, transform);
    else
        m_transform = transform;
}

void Entity::requestTransformParentChange(Entity* newEntityToFollow, bool keepWorldPosition)
{
    // TODO
}

void Entity::handleEditorStateChange(const EntityEditorState& state)
{
    m_editorState = state;
}

void Entity::applyTransform(World* world, const EntityThreadContext& tc, const Transform& localTransform, const Transform& worldTransform)
{
    m_transform = localTransform;
    m_cachedLocalToWorldTransfrom = worldTransform;
    m_cachedLocalToWorldMatrix = m_cachedLocalToWorldTransfrom.toMatrix();

    handleTransformUpdate(tc);
}

void Entity::handleTransformUpdate(const EntityThreadContext& tc)
{
    // usually good place to
}

void Entity::handleAttach()
{
    
}

void Entity::handleDetach()
{
    
}

void Entity::handleDebugRender(rendering::FrameParams& frame) const
{

}

bool Entity::handleInput(const input::BaseEvent& evt)
{
    return false;
}

//--

EntityPtr Entity::updateParent() const
{
    return nullptr;
}

void Entity::handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt)
{

}

void Entity::handleUpdateMask(WorldUpdateMask& outUpdateMask) const
{
    // no update
}

//--

void Entity::handleStreamIn(CompiledStreamingIslandInstance* island)
{
    // nothing
}

void Entity::handleStreamOut(CompiledStreamingIslandInstance* island)
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

void Entity::attachToWorld(World* world, const Transform& localTransform, const Transform& worldTransform, StringID name, EntityRuntimeID runtimeID, EntityStaticID staticID)
{
    DEBUG_CHECK_RETURN(world);
    DEBUG_CHECK_RETURN(!m_world);

    m_transform = localTransform;
    m_cachedLocalToWorldTransfrom = worldTransform;
    m_cachedLocalToWorldMatrix = worldTransform.toMatrix();

    m_staticId = staticID;
    m_runtimeId = runtimeID;
    m_name = name;

    m_world = world;
    handleAttach();
}

void Entity::detachFromWorld(World* world)
{
    DEBUG_CHECK_RETURN(world);
    DEBUG_CHECK_RETURN(m_world == world);

    handleDetach();

    m_staticId = EntityStaticID();
    m_runtimeId = EntityRuntimeID();
    m_name = StringID();

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

    outTemplateProperties.prop("Streaming"_id, "streamingGroupChildren"_id, true, PropertyEditorData().comment("Stream all child entities together as a group (better for logic and visuals, bad for heavy prefabs)"));
    outTemplateProperties.prop("Streaming"_id, "streamingBreakFromGroup"_id, false, PropertyEditorData().comment("Force this entity to stream individually from it's parent (parent still has to be streamed first, there's no override for this)"));
    outTemplateProperties.prop("Streaming"_id, "streamingDistanceOverride"_id, 0.0f, PropertyEditorData().comment("Override distance for the streaming range"));

    outTemplateProperties.prop("Transform"_id, "attachToParentEntity"_id, false, PropertyEditorData().comment("In game follow parent entity"));
}

bool Entity::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
{
    if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
        return false;

    return true;
}

//--

END_BOOMER_NAMESPACE()
