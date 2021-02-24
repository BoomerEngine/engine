/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "base/world/include/worldEntity.h"

BEGIN_BOOMER_NAMESPACE(game)

//--

// generic visual entity that renders something
class GAME_WORLD_API IVisualEntity : public base::world::Entity
{
    RTTI_DECLARE_VIRTUAL_CLASS(IVisualEntity, base::world::Entity);

public:
    IVisualEntity();

    ///---

protected:
    rendering::scene::ObjectProxyPtr m_proxy;

    virtual void handleAttach() override;
    virtual void handleDetach() override;
    virtual void handleTransformUpdate(const base::AbsoluteTransform& transform);
    virtual void handleSelectionChanged() override;

    virtual void queryTemplateProperties(base::ITemplatePropertyBuilder& outTemplateProperties) const override;
    virtual bool initializeFromTemplateProperties(const base::ITemplatePropertyValueContainer& templateProperties) override;

    virtual rendering::scene::ObjectProxyPtr handleCreateProxy(rendering::scene::Scene* scene) const = 0;

    void recreateRenderingProxy();

private:
    void createRenderingProxy();
    void destroyRenderingProxy();
};

//--

END_BOOMER_NAMESPACE(game)
