/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
*
***/

#include "build.h"
#include "sceneWorldParameters_GlobalLight.h"

namespace scene
{

    ///--

    RTTI_BEGIN_TYPE_CLASS(GlobalLightTrajectoryParams);
        RTTI_CATEGORY("Fallback");
        RTTI_PROPERTY(m_defaultPitch).editable("Default light pitch, in case a traveling light is not used");
        RTTI_PROPERTY(m_defaultYaw).editable("Default light yaw, in case a traveling light is not used");
        RTTI_CATEGORY("Debug");
        RTTI_PROPERTY(m_showTrajectories);
    RTTI_END_TYPE();

    GlobalLightTrajectoryParams::GlobalLightTrajectoryParams()
        : m_defaultPitch(15.0f)
        , m_defaultYaw(67.0f)
        , m_showTrajectories(false)
    {}

    void GlobalLightTrajectoryParams::defaultDirection(float pitch, float yaw)
    {
        m_defaultPitch = pitch;
        m_defaultYaw = yaw;
    }

    base::Vector3 GlobalLightTrajectoryParams::calcLightDirection(float timeOfDay) const
    {
        return base::Angles(-m_defaultPitch, m_defaultYaw, 0.0f).forward();
    }

    float GlobalLightTrajectoryParams::calcSunInfluence(float timeOfDay) const
    {
        return 1.0f;
    }

    float GlobalLightTrajectoryParams::calcMoonInfluence(float timeOfDay) const
    {
        return 0.0f;
    }

    void GlobalLightTrajectoryParams::renderDebug(rendering::scene::FrameInfo& info) const
    {
        // TODO: render the trajectories
    }

    ///--

    RTTI_BEGIN_TYPE_CLASS(GlobalLightingParams);
        RTTI_CATEGORY("Lighting");
        RTTI_PROPERTY(m_sunColor).editable("Color of sun color");
        RTTI_PROPERTY(m_sunBrightness).editable("Brightness of the sun lighting");
        RTTI_PROPERTY(m_moonColor).editable("Color of moon color");
        RTTI_PROPERTY(m_moonBrightness).editable("Brightness of the moon lighting");
        RTTI_PROPERTY(m_ambientColor).editable("Default color of the ambient lighting");
    RTTI_END_TYPE();

    GlobalLightingParams::GlobalLightingParams()
        : m_sunColor(255,255,255)
        , m_sunBrightness(2.2f)
        , m_moonColor(255,255,255)
        , m_moonBrightness(0.2f)
        , m_ambientColor(50,50,50)
    {}

    ///--

} // scene


