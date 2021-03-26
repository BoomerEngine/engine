/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "engine/world/include/worldRenderedEntity.h"

BEGIN_BOOMER_NAMESPACE()

//--

// a scene node capable of rendering a mesh
class ENGINE_WORLD_ENTITIES_API MeshEntity : public IWorldRenderedEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshEntity, IWorldRenderedEntity);

public:
    MeshEntity();
    virtual ~MeshEntity();

    INLINE const MeshRef& mesh() const { return m_mesh; }

    INLINE bool castingShadows() const { return m_castShadows; }
    INLINE bool receivingShadows() const { return m_receiveShadows; }

    INLINE char forcedDetailLevel() const { return m_forceDetailLevel; }
    INLINE uint16_t forcedDistance() const { return m_forceDistance; }
    INLINE Color color() const { return m_color; }
    INLINE Color colorEx() const { return m_colorEx; }

    ///--

    void castingShadows(bool flag);
    void receivingShadows(bool flag);

    void forceDetailLevel(char level);
    void forceDistance(float distance);

    void color(Color color);
    void colorEx(Color color);

    void mesh(const MeshRef& mesh);

    ///--

    virtual Box calcBounds() const override;

protected:
    MeshRef m_mesh;
        
    Color m_color = Color::WHITE;
    Color m_colorEx = Color::BLACK;

    bool m_castShadows = true;
    bool m_receiveShadows = true;
    char m_forceDetailLevel = 0;
    uint16_t m_forceDistance = 0;

    //--

    virtual void createRenderingProxy(RenderingScene* scene, RenderingObjectPtr& outProxy, IRenderingObjectManager*& outManager) const override;

    virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const override;
    virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties) override;
};

//--

END_BOOMER_NAMESPACE()
