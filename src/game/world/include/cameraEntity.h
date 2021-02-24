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

// a scene component that is usable as a camera
class GAME_WORLD_API CameraEntity : public base::world::Entity
{
    RTTI_DECLARE_VIRTUAL_CLASS(CameraEntity, base::world::Entity);

public:
    CameraEntity();
    virtual ~CameraEntity();

    ///--

    /// camera FOV
    INLINE float fov() const { return m_fov; }

    /// camera custom near plane
    INLINE float nearPlane() const { return m_forceNearPlane; }

    /// camera custom far plane
    INLINE float farPlane() const { return m_forceFarPlane; }

    ///--

    // set camera fov
    void fov(float value);

    // set camera custom near plane
    void nearPlane(float value);

    // set camera custom far plane
    void farPlane(float value);

    ///---

    /// render world from this camera
    //void render(const CameraRenderingContext& context, const CameraRenderingTarget& target);

    ///---

protected:
    float m_forceNearPlane = 0.0f;
    float m_forceFarPlane = 0.0f;
    float m_fov = 90.0f;
};

//--

END_BOOMER_NAMESPACE(game)
