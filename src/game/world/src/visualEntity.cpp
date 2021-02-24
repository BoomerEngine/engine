/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "visualEntity.h"

#include "rendering/world/include/renderingSystem.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneCommand.h"
#include "rendering/world/include/renderingSystem.h"

BEGIN_BOOMER_NAMESPACE(game)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IVisualEntity);
RTTI_END_TYPE();

IVisualEntity::IVisualEntity()
{
}
    
void IVisualEntity::handleAttach()
{
    TBaseClass::handleAttach();
    createRenderingProxy();
}

void IVisualEntity::handleDetach()
{
    TBaseClass::handleDetach();
    destroyRenderingProxy();
}

void IVisualEntity::recreateRenderingProxy()
{
    if (attached())
    {
        destroyRenderingProxy();
        createRenderingProxy();
    }
}
    
void IVisualEntity::createRenderingProxy()
{
    DEBUG_CHECK_RETURN(!m_proxy);

    if (auto* scene = system<rendering::RenderingSystem>())
    {
        m_proxy = handleCreateProxy(scene->scene());

        if (m_proxy)
            scene->scene()->attachProxy(m_proxy);
    }
}

void IVisualEntity::destroyRenderingProxy()
{
    if (m_proxy)
    {
        if (auto* scene = system<rendering::RenderingSystem>())
            scene->scene()->dettachProxy(m_proxy);

        m_proxy.reset();
    }
}

void IVisualEntity::handleTransformUpdate(const base::AbsoluteTransform& transform)
{
    TBaseClass::handleTransformUpdate(transform);

    if (m_proxy)
    {
        if (auto* scene = system<rendering::RenderingSystem>())
        {
            scene->scene()->moveProxy(m_proxy, localToWorld());
        }
    }
}

void IVisualEntity::handleSelectionChanged()
{
    TBaseClass::handleSelectionChanged();

    if (m_proxy)
    {
        if (auto* scene = system<rendering::RenderingSystem>())
        {
            rendering::scene::ObjectProxyFlags clearFlags, setFlags;

            if (selected())
                setFlags |= rendering::scene::ObjectProxyFlagBit::Selected;
            else
                clearFlags |= rendering::scene::ObjectProxyFlagBit::Selected;

            scene->scene()->changeProxyFlags(m_proxy, clearFlags, setFlags);
        }
    }
}

bool IVisualEntity::initializeFromTemplateProperties(const base::ITemplatePropertyValueContainer& templateProperties)
{
    if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
        return false;

    return true;
}

void IVisualEntity::queryTemplateProperties(base::ITemplatePropertyBuilder& outTemplateProperties) const
{
    TBaseClass::queryTemplateProperties(outTemplateProperties);
}

//--

END_BOOMER_NAMESPACE(game)