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

        struct RENDERING_SCENE_API SceneViewStats
        {
            uint32_t numViews = 0; // actual counts for this view type
            double cullingTime = 0.0; // total culling time in this view(s)
            double fragmentsTime = 0.0; // total fragment generation time in this view(s)
            
            base::SpinLock mergeLock;

            //--

            SceneViewStats();
            void merge(const SceneViewStats& view);
        };

        struct RENDERING_SCENE_API SceneStats
        {
            uint32_t numScenes = 0;
            SceneViewStats mainViews;

            //--

            SceneStats();

            void reset();
            void merge(const SceneStats& other);
        };

        ///--

        struct RENDERING_SCENE_API FrameViewStats
        {
            base::SpinLock mergeLock;

            uint32_t numViews = 0; // actual counts for this view type
            double recordTime = 0.0; // total command buffer recording time in this view(s)

            //--

            FrameViewStats();

            void merge(const FrameViewStats& other);
        };

        struct RENDERING_SCENE_API FrameStats
        {
            FrameViewStats mainViews;

            //--

            FrameStats();

            void reset();
            void merge(const FrameStats& other);
        };

        //--

        // render scene stats gui (ImGui)
        extern RENDERING_SCENE_API void RenderStatsGui(const FrameStats& frameStats, const SceneStats& mergedSceneStats);

        //--

    } // scene
} // rendering

