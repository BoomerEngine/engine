/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "sceneWorldParameters.h"

namespace scene
{

    ///----

    /// trajectory of global lighting (sun/moon)
    class SCENE_COMMON_API GlobalLightTrajectoryParams : public IWorldParameters
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GlobalLightTrajectoryParams, IWorldParameters);

    public:
        GlobalLightTrajectoryParams();

        /// set default light direction
        void defaultDirection(float pitch, float yaw);

        /// compute light direction at given time of day
        base::Vector3 calcLightDirection(float timeOfDay) const;

        /// compute the sun influence (normalized)
        float calcSunInfluence(float timeOfDay) const;

        /// compute the moon influence (normalized)
        float calcMoonInfluence(float timeOfDay) const;

    private:
        float m_defaultPitch;
        float m_defaultYaw;

        bool m_showTrajectories;

        /// render debug stuff related to the parameters into the viewport
        virtual void renderDebug(rendering::scene::FrameInfo& info) const;
    };

    ///----

    /// global lighting
    class SCENE_COMMON_API GlobalLightingParams : public IWorldParameters
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GlobalLightingParams, IWorldParameters);

    public:
        GlobalLightingParams();

        base::Color m_sunColor;
        float m_sunBrightness;

        base::Color m_moonColor;
        float m_moonBrightness;

        base::Color m_ambientColor;
    };

    ///----

} // scene