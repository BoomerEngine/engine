/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: panels #]
***/

#pragma once

#include "base/cooking/include/backgroundBakeService.h"
#include "base/ui/include/uiAbstractItemModel.h"

namespace ed
{
    ///---

    /// list model for listing baked files
    class BASE_EDITOR_API BackgroundBakedListModel : public ui::IAbstractItemModel
    {
    public:
        BackgroundBakedListModel();
        virtual ~BackgroundBakedListModel();

        void handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced);
        void handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime);
        void handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken);

    protected:
        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const ui::ModelIndex& id, int colIndex = 0) const override final;
        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const override final;

        enum class EntryState : uint8_t
        {
            Pending,
            Canceled,
            Processing,
            Finished,
            Failed,
        };

        struct Entry
        {
            res::ResourceKey key;
            base::NativeTimePoint scheduleTime;
            EntryState state = EntryState::Pending;
            float waitTime = FLT_MAX;
            float processingTime = FLT_MAX;
            int index = -1;

            static const auto NUM_COLUMNS = 6;
            static const auto COL_TYPE = 0; // auto/forced
            static const auto COL_CLASS = 1;
            static const auto COL_PATH = 2;
            static const auto COL_STATE = 3;
            static const auto COL_PROCESSING_TIME = 4;
            static const auto COL_DATA_SIZE = 5;
            base::StringBuf content[NUM_COLUMNS];
        };

        base::Array<Entry*> m_entries;
        base::HashMap<base::res::ResourceKey, Entry*> m_entriesMap;

        Entry* entryForIndex(const ui::ModelIndex& index) const;
        ui::ModelIndex indexForEntry(const Entry* entry) const;
    };

    ///---

    class BASE_EDITOR_API BackgroundBakedListModelNotificataionForwarder : public base::IReferencable, public base::cooker::IBackgroundBakerNotification
    {
    public:
        BackgroundBakedListModelNotificataionForwarder(const base::RefPtr<BackgroundBakedListModel>& model);

    protected:
        virtual void handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced) override final;
        virtual void handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime) override final;
        virtual void handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken) override final;
        virtual void handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key) override final;

        base::RefPtr<BackgroundBakedListModel> m_model;
    };

    ///---

    /// panel displaying the background baking list
    class BASE_EDITOR_API BackgroundBakerPanel : public ui::DockPanel, public base::cooker::IBackgroundBakerNotification
    {
        RTTI_DECLARE_VIRTUAL_CLASS(BackgroundBakerPanel, ui::DockPanel);

    public:
        BackgroundBakerPanel();
        virtual ~BackgroundBakerPanel();

        void loadConfig(const ConfigGroup& config);
        void saveConfig(ConfigGroup config) const;

    private:
        ui::ListViewPtr m_fileList;

        void cmdToggleBackgroundBake();
        bool checkBackgroundBake() const;

        base::RefPtr<BackgroundBakedListModel> m_listModel;
        base::RefPtr<BackgroundBakedListModelNotificataionForwarder> m_listForwarder;

        virtual void handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced) override final;
        virtual void handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime) override final;
        virtual void handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken) override final;
        virtual void handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key) override final;
    };

    ///---

} // editor

