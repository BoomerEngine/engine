/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: panels #]
***/

#include "build.h"
#include "backgroundBakerPanel.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/ui/include/uiAbstractItemView.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiTrackBar.h"
#include "base/ui/include/uiColumnHeaderBar.h"

namespace ed
{
    //--

    BackgroundBakedListModel::BackgroundBakedListModel()
    {}

    BackgroundBakedListModel::~BackgroundBakedListModel()
    {}

    void BackgroundBakedListModel::handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced)
    {
        if (!key.empty())
        {
            Entry* entry = nullptr;
            if (m_entriesMap.find(key, entry))
            {
                if (entry->state == EntryState::Processing)
                {
                    entry->state = EntryState::Canceled;
                    entry->content[Entry::COL_STATE] = "[color:#AAA][i]Canceled";
                    requestItemUpdate(indexForEntry(entry));
                }
                else if (entry->state == EntryState::Pending)
                {
                    return;
                }
            }

            entry = MemNew(Entry);
            entry->key = key;
            entry->state = EntryState::Pending;
            entry->index = m_entries.size();
            entry->scheduleTime.resetToNow();

            static const auto firstTime = NativeTimePoint::Now();
            entry->content[Entry::COL_TYPE] = forced ? "Forced" : "Auto";
            entry->content[Entry::COL_PATH] = base::TempString("[img:file] {}", key.path().path());
            entry->content[Entry::COL_CLASS] = base::StringBuf(key.cls()->name().view().afterLastOrFull("::"));
            entry->content[Entry::COL_STATE] = "[color:#AAA][i]Pending";
            entry->content[Entry::COL_PROCESSING_TIME] = "[color:#AAA][i]---";

            beingInsertRows(ui::ModelIndex(), m_entries.size(), 1);
            m_entries.pushBack(entry);
            m_entriesMap[key] = entry;
            endInsertRows();
        }
    }

    void BackgroundBakedListModel::handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime)
    {
        Entry* entry = nullptr;
        if (m_entriesMap.find(key, entry))
        {
            entry->state = EntryState::Processing;
            entry->waitTime = waitTime;
            entry->content[Entry::COL_STATE] = "[color:#0FF][b]Processing";
            requestItemUpdate(indexForEntry(entry));
        }
    }

    void BackgroundBakedListModel::handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken)
    {
        Entry* entry = nullptr;
        if (m_entriesMap.find(key, entry))
        {
            entry->processingTime = timeTaken;
            if (valid)
            {
                entry->state = EntryState::Finished;
                entry->content[Entry::COL_STATE] = "[color:#0F0][b]Finished";
            }
            else
            {
                entry->state = EntryState::Failed;
                entry->content[Entry::COL_STATE] = "[color:#F00][b]Failed";
            }
            entry->content[Entry::COL_PROCESSING_TIME] = base::TempString("{}", TimeInterval(timeTaken));
            requestItemUpdate(indexForEntry(entry));
        }
    }

    uint32_t BackgroundBakedListModel::rowCount(const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (!parent.valid())
            return m_entries.size();
        return 0;
    }

    bool BackgroundBakedListModel::hasChildren(const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        return !parent.valid();
    }

    bool BackgroundBakedListModel::hasIndex(int row, int col, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        return !parent.valid() && col == 0 && row >= 0 && row < (int)m_entries.size();
    }

    ui::ModelIndex BackgroundBakedListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    ui::ModelIndex BackgroundBakedListModel::index(int row, int column, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (hasIndex(row, column, parent))
            return ui::ModelIndex(this, row, column);
        return ui::ModelIndex();
    }

    ui::ModelIndex BackgroundBakedListModel::indexForEntry(const Entry* entry) const
    {
        if (entry && entry->index != -1)
            return ui::ModelIndex(this, entry->index, 0);
        return ui::ModelIndex();
    }

    BackgroundBakedListModel::Entry* BackgroundBakedListModel::entryForIndex(const ui::ModelIndex& index) const
    {
        if (index.row() >= 0 && index.row() <= m_entries.lastValidIndex())
            return m_entries[index.row()];
        return nullptr;
    }

    bool BackgroundBakedListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        const auto* firstEntry = entryForIndex(first);
        const auto* secondEntry = entryForIndex(second);
        if (firstEntry && secondEntry)
        {
            switch (colIndex)
            {
                case Entry::COL_STATE: return (int)firstEntry->state < (int)secondEntry->state;
                case Entry::COL_PROCESSING_TIME: return firstEntry->processingTime < secondEntry->processingTime;

                default:
                    return firstEntry->content[colIndex] < secondEntry->content[colIndex];
            }
        }

        return firstEntry < secondEntry;
    }

    bool BackgroundBakedListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (const auto* entry = entryForIndex(id))
            return filter.testString(entry->key.path().path());
        return false;
    }

    base::StringBuf BackgroundBakedListModel::displayContent(const ui::ModelIndex& id, int colIndex /*= 0*/) const
    {
        if (const auto* entry = entryForIndex(id))
            return entry->content[colIndex];
        return base::StringBuf::EMPTY();
    }

    ui::PopupPtr BackgroundBakedListModel::contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const
    {
        return nullptr;
    }

    //--

    BackgroundBakedListModelNotificataionForwarder::BackgroundBakedListModelNotificataionForwarder(const base::RefPtr<BackgroundBakedListModel>& model)
        : m_model(model)
    {}

    /*void BackgroundBakedListModelNotificataionForwarder::handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced)
    {
        auto selfRef = m_model.weak();
        RunSync("ModelListUpdate") << [selfRef, key, forced](FIBER_FUNC)
        {
            if (auto model = selfRef.lock())
                model->handleBackgroundBakingRequested(key, forced);
        };
    }

    void BackgroundBakedListModelNotificataionForwarder::handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime)
    {
        auto selfRef = m_model.weak();
        RunSync("ModelListUpdate") << [selfRef, key, waitTime](FIBER_FUNC)
        {
            if (auto model = selfRef.lock())
                model->handleBackgroundBakingStarted(key, waitTime);
        };
    }

    void BackgroundBakedListModelNotificataionForwarder::handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken)
    {
        auto selfRef = m_model.weak();
        RunSync("ModelListUpdate") << [selfRef, key, valid, timeTaken](FIBER_FUNC)
        {
            if (auto model = selfRef.lock())
                model->handleBackgroundBakingFinished(key, valid, timeTaken);
        };
    }

    void BackgroundBakedListModelNotificataionForwarder::handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key)
    {

    }*/

    //--

    RTTI_BEGIN_TYPE_CLASS(BackgroundBakerPanel);
    RTTI_END_TYPE();

    BackgroundBakerPanel::BackgroundBakerPanel()
        : ui::DockPanel("[img:cog] Background Bake")
    {
        layoutVertical();

        actions().bindCommand("Baker.ToggleBackgroundBake"_id) = [this]() { cmdToggleBackgroundBake(); };
        actions().bindToggle("Baker.ToggleBackgroundBake"_id) = [this]() { return checkBackgroundBake(); };

        // toolbar
        {
            auto toolbar = createChild<ui::ToolBar>();
            //toolbar->createButton("Baker.ToggleBackgroundBake"_id, "[img:cog] Enable Bake", "Enable background bake of assets");
            //toolbar->createSeparator();

            auto trackbar = toolbar->createChild<ui::TrackBar>();
            trackbar->range(1, 8);
            trackbar->value(2);
            trackbar->resolution(0);
            trackbar->customStyle<float>("width"_id, 150.0f);
        }

        // list
        {
            auto bar = createChild<ui::ColumnHeaderBar>();
            bar->addColumn("Type", 60.0f, true);
            bar->addColumn("Class", 150.0f, true);
            bar->addColumn("Path", 700.0f);
            bar->addColumn("State", 100.0f, true);
            bar->addColumn("Time", 100.0f, true);

            m_fileList = createChild<ui::ListView>();
            m_fileList->expand();
            m_fileList->columnCount(5);
        }

        // attach model
        m_listModel = base::CreateSharedPtr<BackgroundBakedListModel>();
        m_listForwarder = base::CreateSharedPtr<BackgroundBakedListModelNotificataionForwarder>(m_listModel);

        //base::GetService<base::res::BackgroundBaker>()->attachListener(m_listForwarder);
        //base::GetService<base::res::BackgroundBaker>()->attachListener(this);

        m_fileList->model(m_listModel);
    }

    BackgroundBakerPanel::~BackgroundBakerPanel()
    {
        //base::GetService<base::res::BackgroundBaker>()->dettachListener(this);
        //base::GetService<base::res::BackgroundBaker>()->dettachListener(m_listForwarder);
    }

    void BackgroundBakerPanel::cmdToggleBackgroundBake()
    {
        //auto flag = base::GetService<base::res::BackgroundBaker>()->enabled();
        //base::GetService<base::res::BackgroundBaker>()->enabled(!flag);
    }

    bool BackgroundBakerPanel::checkBackgroundBake() const
    {
        //return base::GetService<base::res::BackgroundBaker>()->enabled();
        return false;
    }

    void BackgroundBakerPanel::loadConfig(const ConfigGroup& config)
    {

    }

    void BackgroundBakerPanel::saveConfig(ConfigGroup config) const
    {
        
    }

/*    void BackgroundBakerPanel::handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced)
    {
    }

    void BackgroundBakerPanel::handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime)
    {
        PostWindowMessage(this, ui::MessageType::Info, "Bake"_id, base::TempString("[img:cog] Background Baking Started:\n[i]{}[/i]", key.path()));
    }

    void BackgroundBakerPanel::handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken)
    {
        if (!valid)
        {
            PostWindowMessage(this, ui::MessageType::Error, "Bake"_id, base::TempString("[img:error] Background Baking Failed:\n[i]{}[/i]", key.path()));
        }
    }

    void BackgroundBakerPanel::handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key)
    {
    }*/

    //--

} // ed
