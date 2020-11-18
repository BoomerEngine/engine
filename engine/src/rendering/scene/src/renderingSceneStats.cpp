/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneStats.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "base/imgui/include/imgui.h"

namespace rendering
{
    namespace scene
    {
        //--

        SceneCullingProxyStats::SceneCullingProxyStats()
        {}

        void SceneCullingProxyStats::merge(const SceneCullingProxyStats& other)
        {
            numTestedProxies += other.numTestedProxies;
            numCollectedProxies += other.numCollectedProxies;
        }

        //--

        SceneFragmentStats::SceneFragmentStats()
        {}

        void SceneFragmentStats::merge(const SceneFragmentStats& other)
        {
            numFragments += other.numFragments;
        }

        //--

        SceneFragmentBucketStats::SceneFragmentBucketStats()
        {}

        void SceneFragmentBucketStats::merge(const SceneFragmentBucketStats& other)
        {
            for (uint32_t i = 0; i < ARRAY_COUNT(types); ++i)
                types[i].merge(other.types[i]);
        }

        //--

        SceneCullingStats::SceneCullingStats()
        {}

        void SceneCullingStats::merge(const SceneCullingStats& view)
        {
            for (uint32_t i = 0; i < ARRAY_COUNT(proxies); ++i)
                proxies[i].merge(view.proxies[i]);
        }

        //--

        SceneViewStats::SceneViewStats()
        {}

        void SceneViewStats::merge(const SceneViewStats& view)
        {
            auto lock = CreateLock(mergeLock);

            numViews += view.numViews;
            cullingTime += view.cullingTime;
            fragmentsTime += view.fragmentsTime;

            culling.merge(view.culling);

            for (uint32_t i = 0; i < ARRAY_COUNT(buckets); ++i)
                buckets[i].merge(view.buckets[i]);
        }

        SceneStats::SceneStats()
        {}

        void SceneStats::reset()
        {
            memset(this, 0, sizeof(SceneStats));
        }

        void SceneStats::merge(const SceneStats& other)
        {
            numScenes += other.numScenes;
            for (uint32_t i = 0; i < ARRAY_COUNT(views); ++i)
                views[i].merge(other.views[i]);
        }

        //--

        FrameFragmentRenderStats::FrameFragmentRenderStats()
        {}

        void FrameFragmentRenderStats::merge(const FrameFragmentRenderStats& other)
        {
            numBursts += other.numBursts;
            numFragments += other.numFragments;
            numDrawBaches += other.numDrawBaches;
            numDrawTriangles += other.numDrawTriangles;
        }

        //--

        FrameFragmentBucketStats::FrameFragmentBucketStats()
        {}

        void FrameFragmentBucketStats::merge(const FrameFragmentBucketStats& other)
        {
            auto lock = CreateLock(mergeLock);

            for (uint32_t i = 0; i < ARRAY_COUNT(types); ++i)
                types[i].merge(other.types[i]);
        }

        //--

        FrameViewStats::FrameViewStats()
        {}

        void FrameViewStats::merge(const FrameViewStats& other)
        {
            auto lock = CreateLock(mergeLock);

            numViews += other.numViews;
            recordTime += other.recordTime;

            for (uint32_t i = 0; i < ARRAY_COUNT(buckets); ++i)
                buckets[i].merge(other.buckets[i]);
        }

        //--

        FrameStats::FrameStats()
        {}

        void FrameStats::reset()
        {
            memset(this, 0, sizeof(FrameStats));
        }

        void FrameStats::merge(const FrameStats& other)
        {
            for (uint32_t i = 0; i < ARRAY_COUNT(views); ++i)
                views[i].merge(other.views[i]);
        }

        //--

        template<typename T>
        struct MergedStats : public T
        {
            template< uint32_t N >
            INLINE MergedStats<T>(const T (&table)[N])
            {
                for (uint32_t i = 0; i < N; ++i)
                    merge(table[i]);
            }

            MergedStats(const MergedStats<T>& other) = delete;
            MergedStats& operator=(const MergedStats<T>& other) = delete;
        };

        static const char* ProxyName(int type)
        {
            return base::reflection::GetEnumValueName((ProxyType)type);
        }

        static const char* FragmentName(int type)
        {
            return base::reflection::GetEnumValueName((FragmentHandlerType)type);
        }

        static const char* ViewTypeName(int type)
        {
            return base::reflection::GetEnumValueName((FrameViewType)type);
        }

        static const char* BucketName(int type)
        {
            return base::reflection::GetEnumValueName((FragmentDrawBucket)type);
        }
        
        struct StaticGUISettings
        {
            bool breakDownPerFragmentType = false;
            bool breakDownPerBucketType = false;
            bool breakDownPerProxyType = false;
            bool breakDownPerViewType = false;
        };

        static StaticGUISettings GGuiSettings;

        void RenderStatsGuid(const SceneCullingProxyStats& stat)
        {
            //ImGui::SetNextItemWidth(300);
            //ImGui::ProgressBar(frac, ImVec2(-1, 0), base::TempString("{} / {}", MemSize(size), MemSize(max)));
            ImGui::Text("%u tested, %u visible", stat.numTestedProxies, stat.numCollectedProxies);
        }

        void RenderStatsGui(const SceneCullingStats& stats)
        {
            MergedStats<SceneCullingProxyStats> mergedProxyStats(stats.proxies);
            
            if (GGuiSettings.breakDownPerProxyType)
            {
                bool opened = ImGui::TreeNodeEx("##All", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5, 1, 1), "All proxy types");
                if (opened)
                {
                    RenderStatsGuid(mergedProxyStats);
                    ImGui::TreePop();
                }

                for (uint32_t i = 1; i < NUM_PROXY_TYPES; ++i)
                {
                    if (ImGui::TreeNode(ProxyName(i)))
                    {
                        RenderStatsGuid(stats.proxies[i]);
                        ImGui::TreePop();
                    }
                }
            }
            else
            {
                ImGui::Text("All proxies: ");
                ImGui::SameLine();
                RenderStatsGuid(mergedProxyStats);
            }
        }

        void RenderStatsGui(const FrameFragmentRenderStats& frameType, const SceneFragmentStats& sceneType)
        {
            ImGui::Text("%d rendered, %d generated", frameType.numFragments, sceneType.numFragments);
            ImGui::Text("%d bursts, %d batches", frameType.numBursts, frameType.numDrawBaches);
            ImGui::Text("%d triangles", frameType.numDrawTriangles);
        }

        void RenderStatsGui(const FrameFragmentBucketStats& frameBuckets, const SceneFragmentBucketStats& sceneBuckets)
        {
            MergedStats<FrameFragmentRenderStats> mergedFrameTypes(frameBuckets.types);
            MergedStats<SceneFragmentStats> mergedSceneTypes(sceneBuckets.types);

            if (GGuiSettings.breakDownPerFragmentType)
            {
                bool opened = ImGui::TreeNodeEx("##All", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5, 0.5, 1), "All fragment types");
                if (opened)
                {
                    RenderStatsGui(mergedFrameTypes, mergedSceneTypes);
                    ImGui::TreePop();
                }

                for (uint32_t i = 1; i < NUM_FRAGMENT_TYPES; i++)
                {
                    if (ImGui::TreeNode(FragmentName(i)))
                    {
                        RenderStatsGui(frameBuckets.types[i], sceneBuckets.types[i]);
                        ImGui::TreePop();
                    }
                }
            }
            else
            {
                ImGui::Text("All fragment types");
                RenderStatsGui(mergedFrameTypes, mergedSceneTypes);
            }
        }

        void RenderStatsGui(const FrameViewStats& frameView, const SceneViewStats& sceneView)
        {
            ImGui::Text("View count: %d", frameView.numViews);
            ImGui::Text("Culling [us]: %1.0f", sceneView.cullingTime * 1000000.0f);
            ImGui::Text("Gathering [us]: %1.0f", sceneView.fragmentsTime * 1000000.0f);
            ImGui::Text("Recording [us]: %1.0f", frameView.recordTime * 1000000.0f);

            ImGui::Separator();
            RenderStatsGui(sceneView.culling);

            ImGui::Separator();
            MergedStats<FrameFragmentBucketStats> mergedFrameBuckets(frameView.buckets);
            MergedStats<SceneFragmentBucketStats> mergedSceneBuckets(sceneView.buckets);

            if (GGuiSettings.breakDownPerBucketType)
            {
                bool opened = ImGui::TreeNodeEx("##All", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 0.5, 1, 1), "All buckets");
                if (opened)
                {
                    RenderStatsGui(mergedFrameBuckets, mergedSceneBuckets);
                    ImGui::TreePop();
                }

                for (uint32_t i = 0; i < NUM_BUCKET_TYPES; i++)
                {
                    if (ImGui::TreeNode(BucketName(i)))
                    {
                        RenderStatsGui(frameView.buckets[i], sceneView.buckets[i]);
                        ImGui::TreePop();
                    }
                }
            }
            else
            {
                RenderStatsGui(mergedFrameBuckets, mergedSceneBuckets);
            }
        }

        void RenderStatsGui(const FrameStats& frameStats, const SceneStats& mergedSceneStats)
        {
            MergedStats<FrameViewStats> mergedFrameViews(frameStats.views);
            MergedStats<SceneViewStats> mergedSceneViews(mergedSceneStats.views);

            ImGui::Checkbox("PerViews", &GGuiSettings.breakDownPerViewType);
            ImGui::SameLine();
            ImGui::Checkbox("PerBucket", &GGuiSettings.breakDownPerBucketType);
            ImGui::SameLine();
            ImGui::Checkbox("PerProxy", &GGuiSettings.breakDownPerProxyType);
            ImGui::SameLine();
            ImGui::Checkbox("PerFragment", &GGuiSettings.breakDownPerFragmentType);
            ImGui::Separator();

            if (GGuiSettings.breakDownPerViewType)
            {
                bool opened = ImGui::TreeNodeEx("##All views", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 1, 1, 1), "All views");
                if (opened)
                {
                    RenderStatsGui(mergedFrameViews, mergedSceneViews);
                    ImGui::TreePop();
                }

                for (uint32_t i = 0; i < NUM_VIEW_TYPES; i++)
                {
                    if (ImGui::TreeNode(ViewTypeName(i)))
                    {
                        RenderStatsGui(frameStats.views[i], mergedSceneStats.views[i]);
                        ImGui::TreePop();
                    }
                }
            }
            else
            {
                RenderStatsGui(mergedFrameViews, mergedSceneViews);
            }
        }

        //--

    } // scene
} // rendering
