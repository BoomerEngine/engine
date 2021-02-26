/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: pages #]
*
***/

#include "build.h"
#include "debugPage.h"
#include "imgui.h"
#include "core/memory/include/poolStats.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<bool> cvDebugPageMemory("DebugPage.Engine.Memory", "IsVisible", false);

//--

// debug page showing system memory usage and allocator stats
class DebugPage_Memory : public IDebugPage
{
    RTTI_DECLARE_VIRTUAL_CLASS(DebugPage_Memory, IDebugPage);

public:
    DebugPage_Memory()
        : m_prevPoolRange(0)
        , m_secondTimeout(1.0f)
        , m_showAverageTreeStats(false)
        , m_treeStatsMode(0)
    {
        updatePoolList();
    }

    virtual bool handleInitialize() override
    {
        return true;
    }

    virtual void handleTick(float timeDelta) override
    {
        poolStats();
        updatePoolList();

        m_secondTimeout -= timeDelta;
        if (m_secondTimeout < 0.0f)
        {
            m_secondTimeout = 1.0f;
            nextFrame();
        }
    }

    virtual void handleRender() override
    {
        if (!cvDebugPageMemory.get())
            return;

        if (!ImGui::Begin("Memory", &cvDebugPageMemory.get()))
            return;

        if (m_entries.empty())
        {
            ImGui::Text("NO DATA");
            ImGui::End();
            return;
        }
                
        if (ImGui::CollapsingHeader("Global stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // global stats
            auto& rootBlock = m_entries.front();
            ImGui::Text("Total memory allocated: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", MemSize(rootBlock.mergedStats.m_totalSize)));
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::GRAY), TempString("(MAX {})", MemSize(rootBlock.mergedStats.m_maxSize)));
            ImGui::Text("Total allocations: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", rootBlock.mergedStats.m_totalAllocations));
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::GRAY), TempString("(MAX {})", rootBlock.mergedStats.m_maxAllocations));
            ImGui::Separator();
            ImGui::Text("Frame allocations: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", rootBlock.mergedStats.m_lastFrameAllocs));
            ImGui::SameLine(); ImGui::SetCursorPosX(220); ImGui::TextColored(ImColor(Color::LIGHTYELLOW), TempString("({})", MemSize(rootBlock.mergedStats.m_lastFrameAllocSize)));
            ImGui::Text("Frame deallocations: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", rootBlock.mergedStats.m_lastFrameFrees));
            ImGui::SameLine(); ImGui::SetCursorPosX(220); ImGui::TextColored(ImColor(Color::LIGHTYELLOW), TempString("({})", MemSize(rootBlock.mergedStats.m_lastFrameFreesSize)));
            ImGui::Text("Frame delta: ");

            {
                auto color = Color::GRAY;
                if (rootBlock.mergedStats.m_lastFrameAllocs && rootBlock.mergedStats.m_lastFrameAllocs > rootBlock.mergedStats.m_lastFrameFrees)
                    color = Color::RED;
                else if (rootBlock.mergedStats.m_lastFrameFrees && rootBlock.mergedStats.m_lastFrameAllocs < rootBlock.mergedStats.m_lastFrameFrees)
                    color = Color::GREEN;
                ImGui::SameLine();
                ImGui::TextColored(ImColor(color), TempString("{}", (int)(rootBlock.mergedStats.m_lastFrameAllocs - rootBlock.mergedStats.m_lastFrameFrees)));
            }

            {
                auto color = Color::GRAY;
                if (rootBlock.mergedStats.m_lastFrameAllocSize && rootBlock.mergedStats.m_lastFrameAllocSize > rootBlock.mergedStats.m_lastFrameFreesSize)
                    color = Color::RED;
                else if (rootBlock.mergedStats.m_lastFrameFreesSize && rootBlock.mergedStats.m_lastFrameAllocSize < rootBlock.mergedStats.m_lastFrameFreesSize)
                    color = Color::GREEN;
                ImGui::SameLine();
                ImGui::TextColored(ImColor(color), TempString("{}", (int)(rootBlock.mergedStats.m_lastFrameAllocSize - rootBlock.mergedStats.m_lastFrameFreesSize)));
            }

            ImGui::Separator();
            ImGui::Text("Second allocations: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", rootBlock.lastFrameStats.m_lastFrameAllocs));
            ImGui::SameLine(); ImGui::SetCursorPosX(220); ImGui::TextColored(ImColor(Color::LIGHTYELLOW), TempString("({})", MemSize(rootBlock.lastFrameStats.m_lastFrameAllocSize)));
            ImGui::Text("Second deallocations: ");
            ImGui::SameLine(); ImGui::TextColored(ImColor(Color::LIGHTCYAN), TempString("{}", rootBlock.lastFrameStats.m_lastFrameFrees));
            ImGui::SameLine(); ImGui::SetCursorPosX(220); ImGui::TextColored(ImColor(Color::LIGHTYELLOW), TempString("({})", MemSize(rootBlock.lastFrameStats.m_lastFrameFreesSize)));
            ImGui::Text("Second delta: ");

            {
                auto color = Color::GRAY;
                if (rootBlock.lastFrameStats.m_lastFrameAllocs && rootBlock.lastFrameStats.m_lastFrameAllocs > rootBlock.lastFrameStats.m_lastFrameFrees)
                    color = Color::RED;
                else if (rootBlock.lastFrameStats.m_lastFrameFrees && rootBlock.lastFrameStats.m_lastFrameAllocs < rootBlock.lastFrameStats.m_lastFrameFrees)
                    color = Color::GREEN;
                ImGui::SameLine();
                ImGui::TextColored(ImColor(color), TempString("{}", (int)(rootBlock.lastFrameStats.m_lastFrameAllocs - rootBlock.lastFrameStats.m_lastFrameFrees)));
            }

            {
                auto color = Color::GRAY;
                if (rootBlock.lastFrameStats.m_lastFrameAllocSize && rootBlock.lastFrameStats.m_lastFrameAllocSize > rootBlock.lastFrameStats.m_lastFrameFreesSize)
                    color = Color::RED;
                else if (rootBlock.lastFrameStats.m_lastFrameFreesSize && rootBlock.lastFrameStats.m_lastFrameAllocSize < rootBlock.lastFrameStats.m_lastFrameFreesSize)
                    color = Color::GREEN;
                ImGui::SameLine();
                ImGui::TextColored(ImColor(color), TempString("{}", (int)(rootBlock.lastFrameStats.m_lastFrameAllocSize - rootBlock.lastFrameStats.m_lastFrameFreesSize)));
            }
        }

        if (ImGui::CollapsingHeader("Pool stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* txt[4] = { "Total", "Allocations", "Deallocations", "Delta" };
            ImGui::Checkbox("Show average stats", &m_showAverageTreeStats);
            ImGui::Combo("StatMode", &m_treeStatsMode, txt, 4);

            for (uint32_t i = 0; i < m_entries.size(); ++i)
            {
                if (m_entries[i].parent == 0)
                    emitPoolInfo(m_showAverageTreeStats, m_treeStatsMode, m_entries.typedData(), i);
            }
        }

        ImGui::End();
    }

private:
    struct PoolInfo
    {
        StringID name;
        PoolTag poolID;
        bool poolIDValid = false;
        int parent = -1;
        Array<short> children;
        mem::PoolStatsData localStats;
        mem::PoolStatsData mergedStats;

        mem::PoolStatsData curFrameStats;
        mem::PoolStatsData lastFrameStats;
    };

    void emitPoolInfo(bool average, int mode, const PoolInfo* infos, short id)
    {
        auto& info = infos[id];
        auto& stats = average ? info.lastFrameStats : info.mergedStats;

        int64_t num = 0;
        int64_t max = 0;
        int64_t size = 0;
        if (mode == 0)
        {
            num = stats.m_totalAllocations;
            size = stats.m_totalSize;
            max = stats.m_maxSize;
        }
        else if (mode == 1)
        {
            num = stats.m_lastFrameAllocs;
            size = stats.m_lastFrameAllocSize;
        }
        else if (mode == 2)
        {
            num = -(int64_t)stats.m_lastFrameFrees;
            size = -(int64_t)stats.m_lastFrameFreesSize;
        }
        else if (mode == 3)
        {
            num = (int64_t)stats.m_lastFrameAllocs - (int64_t)stats.m_lastFrameFrees;
            size = (int64_t)stats.m_lastFrameAllocSize - (int64_t)stats.m_lastFrameFreesSize;
        }

        if (info.children.empty())
        {
            if (mode == 0)
            {
                ImGui::Text(TempString("{} ({})", info.name, num));
                auto frac = max ? (float) size / (float) max : 0.0f;
                ImGui::SameLine(200.0f);
                ImGui::ProgressBar(frac, ImVec2(-1, 0), TempString("{} / {}", MemSize(size), MemSize(max)));
            }
            else
            {
                auto color = Color::WHITE;
                if (num < 0)
                    color = Color::GREEN;
                else if (num > 0)
                    color = Color::RED;
                ImGui::TextColored(ImColor(color), TempString("{} ({}) [{}]", info.name, num, MemSize(size)));
            }
        }
        else
        {
            if (ImGui::TreeNode(&info, TempString("{} ({}) ({})", info.name, num, MemSize(size))))
            {
                for (auto childId : info.children)
                    emitPoolInfo(average, mode, infos, childId);
                ImGui::TreePop();
            }
        }
    }

    uint32_t m_prevPoolRange;
    Array<PoolInfo> m_entries;

    bool m_showAverageTreeStats;
    int m_treeStatsMode;

    float m_secondTimeout;

    int createPool(int parentID, StringView name)
    {
        auto nameStr = StringID(name);

        for (uint32_t i=0; i<m_entries.size(); ++i)
        {
            auto& entry = m_entries[i];
            if (entry.parent == parentID && entry.name == name)
                return i;
        }

        auto id = (int)m_entries.size();
        auto& entry = m_entries.emplaceBack();
        entry.parent = parentID;
        entry.name = nameStr;

        if (parentID != INDEX_NONE)
            m_entries[parentID].children.pushBack(id);

        return id;
    }

    int createPoolPath(StringView fullName)
    {
        InplaceArray<StringView, 4> parts;
        fullName.slice(".", false, parts);

        int id = createPool(INDEX_NONE, "Root");
        for (auto& name : parts)
            id = createPool(id, name);

        return id;
    }

    static void MergeChildStats(mem::PoolStatsData& merge, const  mem::PoolStatsData& child)
    {
        merge.m_totalAllocations += child.m_totalAllocations;
        merge.m_maxAllocations += child.m_maxAllocations;
        merge.m_totalSize += child.m_totalSize;
        merge.m_maxSize += child.m_maxSize;
        merge.m_maxAllowedSize += child.m_maxAllowedSize;
        //merge.m_totalPages += child.m_totalPages;
        //merge.m_maxPages += child.m_maxPages;
        //merge.m_freePages += child.m_freePages;
        merge.m_lastFrameAllocs += child.m_lastFrameAllocs;
        merge.m_lastFrameFrees += child.m_lastFrameFrees;
        merge.m_lastFrameAllocSize += child.m_lastFrameAllocSize;
        merge.m_lastFrameFreesSize += child.m_lastFrameFreesSize;
    }

    static void MergeFrameStats(mem::PoolStatsData& merge, const  mem::PoolStatsData& child)
    {
        merge.m_totalAllocations = std::max(merge.m_totalAllocations, child.m_totalAllocations);
        merge.m_maxAllocations = std::max(merge.m_maxAllocations, child.m_maxAllocations);
        merge.m_totalSize = std::max(merge.m_totalSize, child.m_totalSize);
        merge.m_maxSize = std::max(merge.m_maxSize, child.m_maxSize);
        merge.m_maxAllowedSize = std::max(merge.m_maxAllowedSize, child.m_maxAllowedSize);
        //merge.m_totalPages = std::max(merge.m_totalPages, child.m_totalPages);
        //merge.m_maxPages = std::max(merge.m_maxPages, child.m_maxPages);
        //merge.m_freePages = std::min(merge.m_freePages, child.m_freePages);
        merge.m_lastFrameAllocs += child.m_lastFrameAllocs;
        merge.m_lastFrameFrees += child.m_lastFrameFrees;
        merge.m_lastFrameAllocSize += child.m_lastFrameAllocSize;
        merge.m_lastFrameFreesSize += child.m_lastFrameFreesSize;
    }

    void nextFrame()
    {
        for (int i=m_entries.lastValidIndex(); i >=0; --i)
        {
            auto &entry = m_entries[i];
            entry.lastFrameStats = entry.curFrameStats;
            entry.curFrameStats = mem::PoolStatsData();
        }
    }

    void poolStats()
    {
        for (int i=m_entries.lastValidIndex(); i >=0; --i)
        {
            auto& entry = m_entries[i];
            entry.localStats = mem::PoolStatsData();
            if (entry.poolIDValid)
                    mem::PoolStats::GetInstance().stats(entry.poolID, entry.localStats);

            entry.mergedStats = entry.localStats;
            for (auto& childID : entry.children)
            {
                auto& childStats = m_entries[childID];
                MergeChildStats(entry.mergedStats, childStats.mergedStats);
            }

            MergeFrameStats(entry.curFrameStats, entry.mergedStats);
        }
    }

    void updatePoolList()
    {
        auto range = (int)POOL_MAX;
        for (uint32_t i=m_prevPoolRange; i<range; ++i)
        {
            auto name = StringBuf(TempString("Pool{}", i)); // TEMP
            //if (name && *name)
            {
                auto id = createPoolPath(name);
                if (id != INDEX_NONE)
                {
                    m_entries[id].poolID = *(const PoolTag*) &id;
                    m_entries[id].poolIDValid = true;
                }
            }
        }

        m_prevPoolRange = range;
    }
};

RTTI_BEGIN_TYPE_CLASS(DebugPage_Memory);
RTTI_END_TYPE();

END_BOOMER_NAMESPACE()
