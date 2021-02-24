/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "worldComponent_Desprecated.h"

#if 0

        //--

        IComponentLink::IComponentLink(Component* source, Component* target)
            : m_target(target)
            , m_source(source)
        {}

        bool IComponentLink::link()
        {
            DEBUG_CHECK_EX(!m_linked, "Components already linked");
            if (m_linked)
                return true;

            if (!Component::LinkComponents(m_source, m_target, this))
                return false;

            m_linked = true;
            return true;
        }

        void IComponentLink::unlink()
        {
            if (m_linked)
            {
                m_linked = false;
                Component::UnlinkComponents(m_source, m_target, this);
            }
        }

        //--

        ComponentTransformLink::ComponentTransformLink(Component* source, Component* target)
            : IComponentLink(source, target)
        {}

        void ComponentTransformLink::calculateNewAbsoluteTransform(const Transform& componentRelativeTransform, ComponentFlags componentFlags, const AbsoluteTransform& parentTransform, const Matrix& parentToWorld, AbsoluteTransform& outNewAbsoluteTransform, Matrix& outNewLocalToWorldMatrix) const
        {
            if (componentFlags.test(ComponentFlagBit::IdentityRelativeTransform))
            {
                outNewAbsoluteTransform = parentTransform;
                outNewLocalToWorldMatrix = parentToWorld;
            }
            else
            {
                outNewAbsoluteTransform = parentTransform * componentRelativeTransform;
                outNewLocalToWorldMatrix = outNewAbsoluteTransform.approximate();
            }
        }

        //--

        ComponentTransformLinkEx::ComponentTransformLinkEx(Component* source, Component* target, const Transform& relativeTransform, bool updateRotation, bool updateScale)
            : ComponentTransformLink(source, target)
            , m_additionalTransform(relativeTransform)
            , m_updateRotation(updateRotation)
            , m_updateScale(updateScale)
        {
        }

        void ComponentTransformLinkEx::calculateNewAbsoluteTransform(const Transform& componentRelativeTransform, ComponentFlags componentFlags, const AbsoluteTransform& parentTransform, const Matrix& parentToWorld, AbsoluteTransform& outNewAbsoluteTransform, Matrix& outNewLocalToWorldMatrix) const
        {
            ComponentTransformLink::calculateNewAbsoluteTransform(componentRelativeTransform, componentFlags, parentTransform, parentToWorld, outNewAbsoluteTransform, outNewLocalToWorldMatrix);
        }

        //--

        Component::Component()
            : m_entity(nullptr)
        {
            m_localToWorld = Matrix::IDENTITY();
        }

        Component::~Component()
        {
            //DEBUG_CHECK_EX(!m_entity, "Destroying attached component");
            DEBUG_CHECK_EX(!attached(), "Destroying attached component");
        }

        //--

        void Component::updateRelativeTransformFlags()
        {
            if (m_relativeTransform.T == Vector3::ZERO())
                m_flags -= ComponentFlagBit::HasRelativePosition;
            else
                m_flags |= ComponentFlagBit::HasRelativePosition;

            if (m_relativeTransform.R == Quat::IDENTITY())
                m_flags -= ComponentFlagBit::HasRelativeRotation;
            else
                m_flags |= ComponentFlagBit::HasRelativeRotation;

            if (m_relativeTransform.S == Vector3::ONE())
                m_flags -= ComponentFlagBit::HasRelativeScale;
            else
                m_flags |= ComponentFlagBit::HasRelativeScale;

            const ComponentFlags nonIdentityMask = { ComponentFlagBit::HasRelativePosition , ComponentFlagBit::HasRelativeRotation, ComponentFlagBit::HasRelativeScale };
            if (m_flags.anySet(nonIdentityMask))
                m_flags -= ComponentFlagBit::IdentityRelativeTransform;
            else
                m_flags |= ComponentFlagBit::IdentityRelativeTransform;
        }

        void Component::relativeTransform(const Transform& newTransform)
        {
            if (m_relativeTransform != newTransform)
            {
                m_relativeTransform = newTransform;
                updateRelativeTransformFlags();
            }
        }

        void Component::relativePosition(const Vector3& pos)
        {
            if (m_relativeTransform.T != pos)
            {
                m_relativeTransform.T = pos;
                updateRelativeTransformFlags();
                requestTransformUpdate();
            }
        }

        void Component::relativeRotation(const Quat& rot)
        {
            if (m_relativeTransform.R != rot)
            {
                m_relativeTransform.R = rot;
                updateRelativeTransformFlags();
                requestTransformUpdate();
            }
        }

        void Component::relativeScale(const Vector3& scale)
        {
            if (m_relativeTransform.S != scale)
            {
                m_relativeTransform.S = scale;
                updateRelativeTransformFlags();
                requestTransformUpdate();
            }
        }

        //--

        ComponentTransformLinkPtr Component::unlinkTransform(bool preserveWorldPosition /*= true*/)
        {
            auto* link = transformLink();
            if (!link)
                return nullptr;

            ComponentTransformLinkPtr ret = AddRef(link);
            ret->unlink();

            return ret;
        }

        ComponentTransformLinkPtr Component::linkTransform(Component* transformSource, bool preserveWorldPosition /*= true*/)
        {
            const auto prev = unlinkTransform(preserveWorldPosition);

            auto link = RefNew<ComponentTransformLink>(transformSource, this);
            if (link->link())
                return link;

            if (prev)
                prev->link(); // hope this works...

            return nullptr;
        }

        ComponentTransformLinkPtr Component::linkTransformEx(Component* transformSource, Transform& relativeTransform, bool updateRotation /*= true*/, bool updateScale /*= true*/)
        {
            // TODO
            return linkTransform(transformSource);
        }

        //--

        void Component::breakIncomingLinks()
        {
            if (m_links)
            {
                InplaceArray<ComponentWeakLinkPtr, 128> weakLinks;
                for (const auto& link : m_links->m_incomingLinks)
                    weakLinks.pushBack(link.get());

                for (const auto& weakLink : weakLinks)
                {
                    if (auto* link = weakLink.unsafe())
                        link->unlink();
                }
            }
        }

        void Component::breakOutgoingLinks()
        {
            if (m_links)
            {
                InplaceArray<ComponentWeakLinkPtr, 128> weakLinks;
                for (const auto& link : m_links->m_outgoingLinks)
                    weakLinks.pushBack(link.get());

                for (const auto& weakLink : weakLinks)
                {
                    if (auto* link = weakLink.unsafe())
                        link->unlink();
                }
            }
        }

        void Component::breakAllLinks()
        {
            if (m_links)
            {
                InplaceArray<ComponentWeakLinkPtr, 128> weakLinks;
                for (const auto& link : m_links->m_outgoingLinks)
                    weakLinks.pushBack(link.get());
                for (const auto& link : m_links->m_incomingLinks)
                    weakLinks.pushBack(link.get());

                for (const auto& weakLink : weakLinks)
                {
                    if (auto* link = weakLink.unsafe())
                        link->unlink();
                }
            }
        }

        //--

        void Component::handleDebugRender(rendering::scene::FrameParams& frame) const
        {
            // TODO: draw component center 
        }

        //--

        void Component::toggleSelectionHighlight(bool flag)
        {
            if (flag != selected())
            {
                if (flag)
                    m_flags.set(ComponentFlagBit::EffectSelectionHighlight);
                else
                    m_flags.clear(ComponentFlagBit::EffectSelectionHighlight);

                handleSelectionHighlightChanged();
            }
        }

        void Component::handleSelectionHighlightChanged()
        {

        }

        //--

        Box Component::calcBounds() const
        {
            const auto center = absoluteTransform().position().approximate();
            return Box(center, 0.1f);
        }

        Box Component::calcStreamingBounds() const
        {
            const auto defaultComponentStreamingRange = 75.0f;
            return calcBounds().extruded(defaultComponentStreamingRange);
        }

        //--

        void Component::recalculateTransform(const AbsoluteTransform& parentTransform, const Matrix& parentToWorld)
        {
            if (auto* link = transformLink())
            {
                link->calculateNewAbsoluteTransform(m_relativeTransform, m_flags, parentTransform, parentToWorld, m_absoluteTransform, m_localToWorld);
            }
            else if (m_flags.test(ComponentFlagBit::IdentityRelativeTransform))
            {
                m_absoluteTransform = parentTransform;
                m_localToWorld = parentToWorld;
            }
            else
            {
                m_absoluteTransform = parentTransform * m_relativeTransform;
                m_localToWorld = m_absoluteTransform.approximate();
            }
        }

        void Component::bindName(StringID name)
        {
            DEBUG_CHECK_RETURN(name);
            DEBUG_CHECK_RETURN(m_entity == nullptr);
            m_name = name;
        }

        void Component::requestTransformUpdate()
        {
            if (m_entity)
                m_entity->requestTransformUpdate();
        }

        void Component::handleTransformUpdate(const Component* source, const AbsoluteTransform& parentTransform, const Matrix& parentToWorld)
        {
            recalculateTransform(parentTransform, parentToWorld);

            if (m_links)
            {
                for (auto& outgoingLinks : m_links->m_outgoingTransformLinks)
                {
                    DEBUG_CHECK(outgoingLinks->source() == this);
                    if (auto target = outgoingLinks->target())
                        target->handleTransformUpdate(this, m_absoluteTransform, m_localToWorld);
                }
            }
        }

        World* Component::world() const
        {
            if (m_entity && m_entity->world())
                return m_entity->world();
            return nullptr;
        }

        IWorldSystem* Component::systemPtr(short index) const
        {
            if (m_entity && m_entity->world())
                return m_entity->world()->systemPtr(index);
            return nullptr;
        }

        void Component::detachFromWorld(World* world)
        {
            ASSERT(world);
            ASSERT(m_flags.test(ComponentFlagBit::Attached));
            ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
            ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
            m_flags |= ComponentFlagBit::Detaching;
            handleDetach(world);
            ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
            m_flags -= ComponentFlagBit::Detaching;
        }

        void Component::attachToWorld(World* world)
        {
            ASSERT(world);
            ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
            ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
            m_flags |= ComponentFlagBit::Attaching;
            handleAttach(world);
            ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
            ASSERT(m_flags.test(ComponentFlagBit::Attached));
            m_flags -= ComponentFlagBit::Attaching;
            m_flags |= ComponentFlagBit::Attached;
        }

        void Component::handleAttach(World* scene)
        {
            ASSERT(scene);
            ASSERT(m_flags.test(ComponentFlagBit::Attaching));
            m_flags -= ComponentFlagBit::Attaching;
            m_flags |= ComponentFlagBit::Attached;
        }

        void Component::handleDetach(World* scene)
        {
            ASSERT(scene);
            ASSERT(m_flags.test(ComponentFlagBit::Detaching));
            m_flags -= ComponentFlagBit::Detaching;
            m_flags -= ComponentFlagBit::Attached;
        }

        bool Component::handleIncomingLinkCreated(Component* sourceComponent, IComponentLink* link)
        {
            return true;
        }

        bool Component::handleOutgoingLinkCreated(Component* targetComponent, IComponentLink* link)
        {
            return true;
        }

        void Component::handleIncomingLinkDestroyed(Component* sourceComponent, IComponentLink* link)
        {

        }

        void Component::handleOutgoingLinkDestroyed(Component* targetComponent, IComponentLink* link)
        {

        }

        //--

        SpecificClassType<Component> Component::determineComponentTemplateClass(const ITemplatePropertyValueContainer& templateProperties)
        {
            return templateProperties.compileClass().cast<Component>();
        }

        void Component::queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const
        {
            TBaseClass::queryTemplateProperties(outTemplateProperties);
        }

        bool Component::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
        {
            if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
                return false;

            return true;
        }

        //--

        bool Component::LinkComponents(const ComponentWeakPtr& source, const ComponentWeakPtr& target, IComponentLink* link)
        {
            bool linked = false;

            auto realSource = source.unsafe();
            auto realTarget = target.unsafe();
            if (realSource && realTarget && link)
            {
                if (!realSource->m_links)
                    realSource->m_links.create();

                if (!realTarget->m_links)
                    realTarget->m_links.create();

                const auto existsInSource = realSource->m_links->m_outgoingLinks.contains(link);
                const auto existsInTarget = realTarget->m_links->m_incomingLinks.contains(link);
                DEBUG_CHECK_EX(!existsInSource, "Link already registered");
                DEBUG_CHECK_EX(!existsInTarget, "Link already registered");
                if (!existsInTarget && !existsInTarget)
                {
                    // were were a root component
                    bool targetWasRoot = realTarget->m_entity && realTarget->m_entity->isRootComponent(realTarget);

                    // TODO: handle cases when those callbacks break the link...
                    bool sourceOK = realSource->handleOutgoingLinkCreated(realTarget, link);
                    bool targetOK = realTarget->handleIncomingLinkCreated(realSource, link);

                    // add only AFTER both components agreed
                    if (sourceOK && targetOK)
                    {
                        // target is no longer root
                        if (targetWasRoot)
                            realTarget->m_entity->invalidateRoots();

                        // add to both tables
                        realSource->m_links->m_outgoingLinks.pushBack(AddRef(link));
                        realTarget->m_links->m_incomingLinks.pushBack(AddRef(link));

                        // special handling for transform links since there are the most common ones
                        if (link->is<ComponentTransformLink>())
                        {
                            DEBUG_CHECK_EX(realTarget->m_links->m_transformLink == nullptr, "Target component already has trasnform link");
                            realSource->m_links->m_outgoingTransformLinks.pushBack(static_cast<ComponentTransformLink*>(link));
                            realTarget->m_links->m_transformLink = static_cast<ComponentTransformLink*>(link);
                        }

                        // ok, seems we are lucking enough and we linked
                        linked = true;
                    }
                }
            }

            return linked;
        }

        void Component::UnlinkComponents(const ComponentWeakPtr& source, const ComponentWeakPtr& target, IComponentLink* link)
        {
            auto realSource = source.unsafe();
            auto realTarget = target.unsafe();
            if (realSource && realTarget && link)
            {
                DEBUG_CHECK_EX(realSource->m_links, "No link table, but link removed");
                DEBUG_CHECK_EX(realTarget->m_links, "No link table, but link removed");

                if (realSource->m_links && realTarget->m_links)
                {
                    DEBUG_CHECK_EX(realSource->m_links->m_outgoingLinks.contains(link), "Link not in outgoing table");
                    DEBUG_CHECK_EX(realTarget->m_links->m_incomingLinks.contains(link), "Link not in incoming table");

                    // unregister transform link
                    if (link->is<ComponentTransformLink>())
                    {
                        DEBUG_CHECK_EX(realTarget->m_links->m_transformLink == link, "Target already has different transform link");
                        realTarget->m_links->m_transformLink = nullptr;
                        DEBUG_CHECK_EX(realSource->m_links->m_outgoingTransformLinks.contains(link), "Source does not have this transform link");
                        realSource->m_links->m_outgoingTransformLinks.remove(link);
                    }

                    // remove from table 
                    realSource->m_links->m_outgoingLinks.remove(link);
                    realTarget->m_links->m_incomingLinks.remove(link);

                    // TODO: handle better cases when those callbacks break more links
                    realSource->handleOutgoingLinkCreated(realTarget, link);
                    realTarget->handleIncomingLinkCreated(realSource, link);

                    // maybe target has become root
                    // TODO: better filtering
                    realTarget->m_entity->invalidateRoots();
                }
            }
        }

        //--
#endif
