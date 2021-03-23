/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "worldEntity.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// special entity that contains rendering scene proxy object
class ENGINE_WORLD_API IWorldRenderedEntity : public Entity
{
    RTTI_DECLARE_VIRTUAL_CLASS(IWorldRenderedEntity, Entity);

public:
    IWorldRenderedEntity();
    virtual ~IWorldRenderedEntity();

    //---

    INLINE bool visible() const { return m_visible; }

    INLINE rendering::IObjectProxy* renderingProxyObject() const { return m_proxyObject; }
    INLINE rendering::IObjectManager* renderingProxyManager() const { return m_proxyManager; }

    //---

    // change object visibility, does not destroy the proxy so the object can be shown/hidden right away without any loading or delays
    void changeVisibility(bool visible);

    //---

protected:
    rendering::ObjectProxyPtr m_proxyObject = nullptr;
    rendering::IObjectManager* m_proxyManager = nullptr;

    virtual void createRenderingProxy(rendering::Scene* scene, rendering::ObjectProxyPtr& outProxy, rendering::IObjectManager*& outManager) const = 0;

    virtual void handleAttach() override;
    virtual void handleDetach() override;
    virtual void handleTransformUpdate(const EntityThreadContext& tc) override;
    virtual void handleEditorStateChange(const EntityEditorState& state) override;

    virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const override;
    virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties) override;

    void recreateRenderingProxy();

private:
    bool m_visible = true;

    void createRenderingProxy();
    void destroyRenderingProxy();
};

//---

END_BOOMER_NAMESPACE()
