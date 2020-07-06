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

        static const auto NUM_FRAGMENT_TYPES = (int)FragmentHandlerType::MAX;
        static const auto NUM_BUCKET_TYPES = (int)FragmentDrawBucket::MAX;
        static const auto NUM_PROXY_TYPES = (int)ProxyType::MAX;
        static const auto NUM_VIEW_TYPES = (int)FrameViewType::MAX;

        struct RENDERING_SCENE_API SceneCullingProxyStats
        {
            uint32_t numTestedProxies = 0;
            uint32_t numCollectedProxies = 0;

            //--

            SceneCullingProxyStats();

            void merge(const SceneCullingProxyStats& other);
        };

        struct RENDERING_SCENE_API SceneFragmentStats
        {
            uint32_t numFragments = 0;

            //--

            SceneFragmentStats();

            void merge(const SceneFragmentStats& other);
        };

        struct RENDERING_SCENE_API SceneFragmentBucketStats
        {
            SceneFragmentStats types[NUM_FRAGMENT_TYPES];

            //--

            SceneFragmentBucketStats();

            void merge(const SceneFragmentBucketStats& other);
        };

        struct RENDERING_SCENE_API SceneCullingStats
        {
            SceneCullingProxyStats proxies[NUM_PROXY_TYPES];

            //--

            SceneCullingStats();

            void merge(const SceneCullingStats& view);
        };

        struct RENDERING_SCENE_API SceneViewStats
        {
            uint32_t numViews = 0; // actual counts for this view type
            double cullingTime = 0.0; // total culling time in this view(s)
            double fragmentsTime = 0.0; // total fragment generation time in this view(s)
            
            SceneFragmentBucketStats buckets[NUM_BUCKET_TYPES];
            SceneCullingStats culling;

            base::SpinLock mergeLock;

            //--

            SceneViewStats();
            void merge(const SceneViewStats& view);
        };

        struct RENDERING_SCENE_API SceneStats
        {
            uint32_t numScenes = 0;
            SceneViewStats views[NUM_VIEW_TYPES]; // per view type stats

            //--

            SceneStats();

            void reset();
            void merge(const SceneStats& other);
        };

        ///--

        struct RENDERING_SCENE_API FrameFragmentRenderStats
        {
            uint32_t numBursts = 0;
            uint32_t numFragments = 0;
            uint32_t numDrawBaches = 0;
            uint32_t numDrawTriangles = 0;

            //--

            FrameFragmentRenderStats();

            void merge(const FrameFragmentRenderStats& other);
        };

        struct RENDERING_SCENE_API FrameFragmentBucketStats
        {
            base::SpinLock mergeLock;

            FrameFragmentRenderStats types[NUM_FRAGMENT_TYPES];


            //--

            FrameFragmentBucketStats();

            void merge(const FrameFragmentBucketStats& other);
        };

        struct RENDERING_SCENE_API FrameViewStats
        {
            base::SpinLock mergeLock;

            uint32_t numViews = 0; // actual counts for this view type
            double recordTime = 0.0; // total command buffer recording time in this view(s)

            FrameFragmentBucketStats buckets[NUM_BUCKET_TYPES];

            //--

            FrameViewStats();

            void merge(const FrameViewStats& other);
        };

        struct RENDERING_SCENE_API FrameStats
        {
            FrameViewStats views[NUM_VIEW_TYPES]; // per view type stats

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

