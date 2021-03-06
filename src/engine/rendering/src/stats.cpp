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

BEGIN_BOOMER_NAMESPACE_EX(rendering)

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

void RenderStatsGui(const FrameStats& frameStats)
{
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
            //RenderStatsGui(mergedFrameViews, mergedSceneViews);
            ImGui::TreePop();
        }
    }
    else
    {
        //RenderStatsGui(mergedFrameViews, mergedSceneViews);
    }
}


//--

END_BOOMER_NAMESPACE_EX(rendering)
