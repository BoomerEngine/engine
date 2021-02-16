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
#include "base/memory/include/poolStats.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resourceLoader.h"
#include "base/imgui/include/imgui.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/object/include/globalEventTable.h"

namespace base
{
    //--

    ConfigProperty<bool> cvDebugPageResources("DebugPage.Engine.Resources", "IsVisible", false);

    //--

    // debug page showing resource loading log
    class DebugPage_Resources : public IDebugPage
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DebugPage_Resources, IDebugPage);

    public:
        DebugPage_Resources()
            : m_fileListSortedInvalid(false)
            , m_events(this)
        {
        }

        ~DebugPage_Resources()
        {
            m_events.clear();
            m_fileMap.clearPtr();
        }

        virtual bool handleInitialize()
        {
            auto loadingService = base::GetService<res::LoadingService>();
            if (!loadingService || !loadingService->loader())
                return false;

            if (auto key = loadingService->loader()->eventKey())
            {
                m_events.bind(key, EVENT_RESOURCE_LOADER_FILE_LOADING) = [this](res::ResourcePath data)
                {
                    auto lock = base::CreateLock(m_fileMapLock);
                    updateStatusNoLock(data, FileStatusMode::Loading);
                };

                m_events.bind(key, EVENT_RESOURCE_LOADER_FILE_FAILED) = [this](res::ResourcePath data)
                {
                    auto lock = base::CreateLock(m_fileMapLock);
                    updateStatusNoLock(data, FileStatusMode::Failed);
                };

                m_events.bind(key, EVENT_RESOURCE_LOADER_FILE_UNLOADED) = [this](res::ResourcePath data)
                {
                    auto lock = base::CreateLock(m_fileMapLock);
                    updateStatusNoLock(data, FileStatusMode::Unloaded);
                };

                m_events.bind(key, EVENT_RESOURCE_LOADER_FILE_LOADED) = [this](res::ResourceHandle data)
                {
                    auto lock = base::CreateLock(m_fileMapLock);
                    //updateStatusNoLock(data->key(), FileStatusMode::Loaded);
                };
            }

            return true;
        }

        virtual void handleTick(float timeDelta) override
        {
        }

        virtual void handleRender() override
        {
            if (!cvDebugPageResources.get())
                return;

            if (!ImGui::Begin("Resources", &cvDebugPageResources.get()))
                return;

            auto lock = base::CreateLock(m_fileMapLock);

            if (ImGui::CollapsingHeader("General stats", ImGuiTreeNodeFlags_DefaultOpen))
            {
                uint32_t numLoading = 0;
                uint32_t numLoaded = 0;
                uint32_t numFailed = 0;
                uint32_t numQueued = 0;
                uint32_t numUnloaded = 0;
                uint32_t numUnknown = 0;
                uint32_t numLoadingJobs = 0;
                for (auto info  : m_fileMap.values())
                {
                    //numLoadingJobs += info->lastStatus.m_totalLoadCount;
                    switch (info->lastStatus)
                    {
                        case FileStatusMode::Failed: numFailed += 1; break;
                        case FileStatusMode::Loaded: numLoaded += 1; break;
                        case FileStatusMode::Loading: numLoading += 1; break;
                        //case FileState::Queued: numQueued += 1; break;
                        case FileStatusMode::Unloaded: numUnloaded += 1; break;
                        //case FileState::Unknown: numUnknown += 1; break;
                    }
                }

                ImGui::Text("Files LOADED: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::GREEN), base::TempString("{}", numLoaded));
                ImGui::Text("Files FAILED: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::RED), base::TempString("{}", numFailed));
                ImGui::Text("Files LOADING: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::LIGHTYELLOW), base::TempString("{}", numLoading));
                ImGui::Text("Files UNLOADED: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::GRAY), base::TempString("{}", numUnloaded));
                ImGui::Text("Files QUEUED: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::LIGHTCYAN), base::TempString("{}", numQueued));
                ImGui::Separator();
                ImGui::Text("Total number of loads so far: "); ImGui::SameLine(); ImGui::TextColored(ImColor(base::Color::LIGHTYELLOW), base::TempString("{}", numLoadingJobs));
            }

            if (ImGui::CollapsingHeader("File list", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushItemWidth(-1.0f);
                if (ImGui::ListBoxHeader("##", ImVec2(0, 500)))
                {
                    if (m_fileListSortedInvalid)
                    {
                        m_fileListSortedInvalid = false;
                        std::sort(m_fileListSorted.begin(), m_fileListSorted.end(), [](const FileInfo* a, const FileInfo* b) { return a->lastStatusTimestamp > b->lastStatusTimestamp; } );
                    }

                    for (auto info  : m_fileListSorted)
                    {
                        switch (info->lastStatus)
                        {
                            case FileStatusMode::Failed: ImGui::TextColored(ImColor(base::Color::RED), "FAILED"); break;
                            case FileStatusMode::Loaded: ImGui::TextColored(ImColor(base::Color::GREEN), "LOADED"); break;
                            case FileStatusMode::Loading: ImGui::TextColored(ImColor(base::Color::LIGHTYELLOW), "LOADING"); break;
                            //case FileStatusMode::Queued: ImGui::TextColored(base::Color::LIGHTCYAN, "QUEUED"); break;
                            case FileStatusMode::Unloaded: ImGui::TextColored(ImColor(base::Color::GRAY), "UNLOADED"); break;
                            //case FileStatusMode::Unknown: ImGui::TextColored(base::Color::WHITE, "UNKNOWN"); break;
                        }

                        ImGui::SameLine(100);
                        ImGui::Text(TempString("{}", info->key.view()));
                        //ImGui::SameLine();
                        //ImGui::TextColored(ImColor(base::Color::DARKGRAY), base::TempString("({})", info->key.cls()->name()));
                    }

                    ImGui::ListBoxFooter();
                }
            }

            if (ImGui::CollapsingHeader("Resource loading", ImGuiTreeNodeFlags_DefaultOpen))
            {

            }

            ImGui::End();
        }

    private:
        enum class FileStatusMode : uint8_t
        {
            Loading,
            Loaded,
            Failed,
            Unloaded,
        };

        void updateStatusNoLock(const res::ResourcePath& key, FileStatusMode status)
        {
            auto entry  = m_fileMap[key];
            if (!entry)
            {
                entry = new FileInfo;
                entry->key = key;
                entry->lastStatus = status;
                m_fileMap[entry->key] = entry;
                m_fileListSorted.pushBack(entry);
            }

            entry->lastStatus = status;
            entry->lastStatusTimestamp.resetToNow();
            m_fileListSortedInvalid = true;
        }
   
        struct FileInfo
        {
            RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)

        public:
            base::res::ResourcePath key;
            FileStatusMode lastStatus;
            base::NativeTimePoint lastStatusTimestamp;
        };

        base::HashMap<base::res::ResourcePath, FileInfo*> m_fileMap;
        base::Array<const FileInfo*> m_fileListSorted;
        bool m_fileListSortedInvalid;
        base::Mutex m_fileMapLock;

        GlobalEventTable m_events;
    };

    RTTI_BEGIN_TYPE_CLASS(DebugPage_Resources);
    RTTI_END_TYPE();

    //--

} // plugin