/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "stats.h"
#include "engine/imgui/include/imgui.h"

BEGIN_BOOMER_NAMESPACE()

//--

void FrameViewStats::print(StringView prefix, IFormatStream& f) const
{
    f.appendf("{}NumTriangles: [b]{}[/b][br]", prefix, numTriangles);
    f.appendf("{}NumDraws: [b]{}[/b][br]", prefix, numDrawCalls);
    f.appendf("{}NumChunks: [b]{}[/b][br]", prefix, numChunks);
    f.appendf("{}NumMaterials: [b]{}[/b][br]", prefix, numMaterials);
    f.appendf("{}NumShaders: [b]{}[/b][br]", prefix, numShaders);
    f.appendf("{}TimeCulling: [b]{}[/b][br]", prefix, TimeInterval(cullingTime));
    f.appendf("{}RecCulling: [b]{}[/b][br]", prefix, TimeInterval(recordingTime));
}

void FrameViewStats::merge(const FrameViewStats& stats)
{
    numTriangles += stats.numTriangles;
    numDrawCalls += stats.numDrawCalls;
    numChunks += stats.numChunks;
    numShaders += stats.numShaders;
    numMaterials += stats.numMaterials;
    recordingTime += stats.recordingTime;
    cullingTime += stats.cullingTime;
}

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

struct StaticGUISettings
{
    bool breakDownPerFragmentType = false;
    bool breakDownPerBucketType = false;
    bool breakDownPerProxyType = false;
    bool breakDownPerViewType = false;
};

static StaticGUISettings GGuiSettings;

#if 0

//--

FrameViewStats::FrameViewStats()
{}

void FrameViewStats::merge(const FrameViewStats& other)
{
    auto lock = CreateLock(mergeLock);

    numViews += other.numViews;
    recordTime += other.recordTime;

    /*for (uint32_t i = 0; i < ARRAY_COUNT(buckets); ++i)
        buckets[i].merge(other.buckets[i]);*/
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
    /*for (uint32_t i = 0; i < ARRAY_COUNT(views); ++i)
        views[i].merge(other.views[i]);*/
}

//--



/*void RenderStatsGuid(const SceneCullingProxyStats& stat)
{
    //ImGui::SetNextItemWidth(300);
    //ImGui::ProgressBar(frac, ImVec2(-1, 0), TempString("{} / {}", MemSize(size), MemSize(max)));
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
}*/
#endif

ConfigProperty<bool> cvShowRenderingFrameStats("DebugPage.Rendering.FrameStats", "IsVisible", false);

ConfigProperty<uint32_t> cvRenderingMainDrawCallBudget("Rendering.Budget.Main", "MaxDrawCalls", 5000);
ConfigProperty<uint32_t> cvRenderingMainTrianglesBudget("Rendering.Budget.Main", "MaxTriangles", 30000000);
ConfigProperty<uint32_t> cvRenderingShadowDrawCallBudget("Rendering.Budget.Shadows", "MaxDrawCalls", 3000);
ConfigProperty<uint32_t> cvRenderingShadowTrianglesBudget("Rendering.Budget.Shadows", "MaxTriangles", 20000000);

struct FrameViewBudget
{
    uint32_t maxTriangles = 0;
    uint32_t maxDrawCalls = 0;
};

void RenderViewStatsGui(const FrameViewStats& viewStats, const FrameViewBudget& budgets)
{
    ImGui::Text("Culling [us]: %1.0f", viewStats.cullingTime * 1000000.0f);
    ImGui::Text("Recording [us]: %1.0f", viewStats.recordingTime * 1000000.0f);
    ImGui::Separator();

    if (budgets.maxDrawCalls > 0)
    {
        ImGui::ProgressBar(viewStats.numDrawCalls / (float)budgets.maxDrawCalls);
    }

    if (budgets.maxTriangles > 0)
    {
        ImGui::ProgressBar(viewStats.numTriangles / (float)budgets.maxTriangles);
    }

    ImGui::Separator();

    ImGui::Text("%d draw calls", viewStats.numDrawCalls);
    ImGui::Text("%d triangles", viewStats.numTriangles);
    ImGui::Separator();

    ImGui::Text("%d chunk groups", viewStats.numChunks);
    ImGui::Text("%d shader groups", viewStats.numShaders);
    ImGui::Text("%d material groups", viewStats.numMaterials);
    ImGui::Separator();
}

void RenderStatsGui(const FrameStats& frameStats)
{
    if (cvShowRenderingFrameStats.get() && ImGui::Begin("Frame stats", &cvShowRenderingFrameStats.get()))
    {
        ImGui::Checkbox("PerViews", &GGuiSettings.breakDownPerViewType);
        ImGui::SameLine();

        if (GGuiSettings.breakDownPerViewType)
        {
            {
                bool opened = ImGui::TreeNodeEx("##Main", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 0, 1, 1), "Main view");
                if (opened)
                {
                    FrameViewBudget budgets;
                    budgets.maxDrawCalls = cvRenderingMainDrawCallBudget.get();
                    budgets.maxTriangles = cvRenderingMainTrianglesBudget.get();
                    RenderViewStatsGui(frameStats.mainView, budgets);
                    ImGui::TreePop();
                }
            }

            {
                bool opened = ImGui::TreeNodeEx("##DP", 0);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 0, 1, 1), "Depth prepass");
                if (opened)
                {
                    FrameViewBudget budgets;
                    RenderViewStatsGui(frameStats.depthView, budgets);
                    ImGui::TreePop();
                }
            }

            {
                bool opened = ImGui::TreeNodeEx("##GShadow", 0);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 0, 1, 1), "Global shadows");
                if (opened)
                {
                    FrameViewBudget budgets;
                    budgets.maxDrawCalls = cvRenderingShadowDrawCallBudget.get();
                    budgets.maxTriangles = cvRenderingShadowTrianglesBudget.get();
                    RenderViewStatsGui(frameStats.globalShadowView, budgets);
                    ImGui::TreePop();
                }
            }

            {
                bool opened = ImGui::TreeNodeEx("##LShadow", 0);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 0, 1, 1), "Local shadows");
                if (opened)
                {
                    FrameViewBudget budgets;
                    budgets.maxDrawCalls = cvRenderingShadowDrawCallBudget.get();
                    budgets.maxTriangles = cvRenderingShadowTrianglesBudget.get();
                    RenderViewStatsGui(frameStats.localShadowView, budgets);
                    ImGui::TreePop();
                }
            }

            {
                bool opened = ImGui::TreeNodeEx("##All views", 0);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5, 1, 1, 1), "All views");
                if (opened)
                {
                    RenderViewStatsGui(frameStats.totals, FrameViewBudget());
                    ImGui::TreePop();
                }
            }
        }
        else
        {
            RenderViewStatsGui(frameStats.totals, FrameViewBudget());
        }

        ImGui::End();
    }
}


//--

END_BOOMER_NAMESPACE()
