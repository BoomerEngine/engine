/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {
        ///--

        /// statistics for visibility
        struct RENDERING_SCENE_API SceneVisStats
        {
            uint32_t m_numTestedProxies = 0;
            uint32_t m_numCollectedProxies = 0;

            void mergeOnto(SceneVisStats& outMerged) const;
        };

        /// statistics for a single view within the scene
        struct RENDERING_SCENE_API SceneViewStats
        {
            SceneVisStats m_visStats;

            base::StringID m_name;
            int m_parent = -1;

            uint32_t m_numPointLights = 0;
            uint32_t m_numSpotLights = 0;
            uint32_t m_numAreaLights = 0;

            uint32_t m_numMeshFragments = 0;
            uint32_t m_numMeshDrawCommands = 0;
            uint32_t m_numMeshDrawTriangles = 0;

            uint32_t m_numTerrainFragments = 0;
            uint32_t m_numTerrainDrawCommands = 0;
            uint32_t m_numTerrainDrawTriangles = 0;

            uint32_t m_numMaterialSwitches = 0;
            uint32_t m_numPipelineSwitches = 0;
            uint32_t m_numGeometrySwitches = 0;
            uint32_t m_numVertexFactorySwitches = 0;

            float m_viewCollectTime = 0.0f;
            float m_viewRenderTime = 0.0f;
            float m_viewExecuteTime = 0.0f;

            void mergeOnto(SceneViewStats& outMerged) const;
        };

        ///--

        /// scene rendering statistics
        struct SceneStats
        {
            base::Array<SceneViewStats> m_views;
        };

        ///--

    } // scene
} // rendering

