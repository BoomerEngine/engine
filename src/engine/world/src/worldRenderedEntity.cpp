/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "worldRenderedEntity.h"
#include "worldRendering.h"

#include "engine/rendering/include/object.h"
#include "engine/rendering/include/scene.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldRenderedEntity);
RTTI_END_TYPE();

IWorldRenderedEntity::IWorldRenderedEntity()
{}

IWorldRenderedEntity::~IWorldRenderedEntity()
{}

void IWorldRenderedEntity::handleAttach()
{
    TBaseClass::handleAttach();
    createRenderingProxy();
}

void IWorldRenderedEntity::handleDetach()
{
    TBaseClass::handleDetach();
    destroyRenderingProxy();
}

void IWorldRenderedEntity::recreateRenderingProxy()
{
    if (attached())
    {
        destroyRenderingProxy();
        createRenderingProxy();
    }
}

void IWorldRenderedEntity::createRenderingProxy()
{
    DEBUG_CHECK_RETURN(!m_proxyObject);
    DEBUG_CHECK_RETURN(!m_proxyManager);

    if (auto* scene = system<WorldRenderingSystem>())
    {
        rendering::ObjectProxyPtr proxy;
        rendering::IObjectManager* manager = nullptr;

        createRenderingProxy(scene->scene(), proxy, manager);

        if (proxy && manager)
        {
            manager->commandAttachProxy(proxy);
            m_proxyObject = proxy;
            m_proxyManager = manager;
        }
    }
}

void IWorldRenderedEntity::destroyRenderingProxy()
{
    if (m_proxyObject)
    {
        m_proxyManager->commandDetachProxy(m_proxyObject);

        m_proxyManager = nullptr;
        m_proxyObject.reset();
    }
}

void IWorldRenderedEntity::handleTransformUpdate(const EntityThreadContext& tc)
{
    TBaseClass::handleTransformUpdate(tc);

    if (m_proxyObject)
        m_proxyManager->commandMoveProxy(m_proxyObject, cachedLocalToWorldMatrix());
}

void IWorldRenderedEntity::handleSelectionChanged()
{
    TBaseClass::handleSelectionChanged();

    if (m_proxyObject)
    {
        rendering::ObjectProxyFlags clearFlags, setFlags;

        const auto selected = false; // TODO: selection "service"
        if (selected)
            setFlags |= rendering::ObjectProxyFlagBit::Selected;
        else
            clearFlags |= rendering::ObjectProxyFlagBit::Selected;

        m_proxyManager->commandUpdateProxyFlag(m_proxyObject, clearFlags, setFlags);
    }
}

bool IWorldRenderedEntity::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
{
    if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
        return false;

    return true;
}

void IWorldRenderedEntity::queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const
{
    TBaseClass::queryTemplateProperties(outTemplateProperties);
}

//--

END_BOOMER_NAMESPACE()
