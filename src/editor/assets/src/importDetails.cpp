/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "browserService.h"
#include "importDetails.h"

#include "engine/ui/include/uiListViewEx.h"
#include "engine/ui/include/uiColumnHeaderBar.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiSearchBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

static StringView GetClassDisplayName(ClassType currentClass)
{
    auto name = currentClass.name().view();
    name = name.afterLastOrFull("::");
    return name;
}

StringView ImportStatusToDisplayText(ImportStatus status, bool withIcon = false)
{
    switch (status)
    {
        case ImportStatus::Pending: return "[b][color:#888]Pending[/color]";
        case ImportStatus::Processing: return "[b][color:#FF7]Processing...[/color]";
        case ImportStatus::Checking: return "[b][color:#AAA]Checking...[/color]";
        case ImportStatus::Canceled: return "[b][tag:#AAA]Canceled[/tag]";
        
        case ImportStatus::FinishedUpTodate:
            if (withIcon)
                return "[b][tag:#88F][img:tick] Up to date[/tag]";
            else
                return "[b][tag:#88F]Up to date[/tag]";

        case ImportStatus::FinishedNewContent:
            if (withIcon)
                return "[b][tag:#8F8][img:cog] [color:#000]Imported[/tag]";
            else
                return "[b][tag:#8F8][color:#000]Imported[/tag]";

        case ImportStatus::Failed:
            if (withIcon)
                return "[b][tag:#F88][color:#000][img:skull] Failed[/tag]";
            else
                return "[b][tag:#F88][color:#000]Failed[/tag]";
    }

    return "[color:#888][b]---[/color]";
}

class AssetImportFileDetails : public ui::IListItem
{
public:
    AssetImportFileDetails(StringView depotPath/*, ResourceClass resourceClass*/)
        : m_fileName(depotPath.fileName())
    {
        layoutColumns();

        {
            auto icon = createChild<ui::TextLabel>("[img:file_add]");
            icon->customMargins(4, 0, 4, 0);
            icon->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            icon->expand();
        }

        /*{
            auto fileClass = createNamedChild<ui::TextLabel>("FileClass"_id);
            fileClass->customMargins(4, 0, 4, 0);
            fileClass->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            fileClass->text(TempString("[img:class] {}", resourceClass));
        }*/

        {
            auto status = createNamedChild<ui::TextLabel>("Status"_id);
            status->customMargins(4, 0, 4, 0);
            status->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            status->expand();

            m_statusLabel = status;
        }

        {
            auto time = createNamedChild<ui::TextLabel>("Time"_id);
            time->customMargins(4, 0, 4, 0);
            time->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            time->expand();

            m_timeLabel = time;
        }

        {
            auto fileName = createNamedChild<ui::TextLabel>("FileNameStatic"_id);
            fileName->customMargins(4, 0, 4, 0);
            fileName->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            fileName->text(TempString("[i]{}", depotPath));
            fileName->expand();
        }
    }

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override
    {
        return filter.testString(m_fileName);
    }

    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outData) const override
    {
        outData.index = uniqueIndex();
        outData.caption = m_fileName;
    }

    virtual bool handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, InputKeyMask controlKeys) override
    {
        auto menu = RefNew<ui::MenuButtonContainer>();

        menu->createCallback("Show in depot...", "[img:zoom]") = [this]() {
            showInAssetBrowser();
        };

        menu->createSeparator();

        menu->show(this);

        return true;
    }

    virtual bool handleItemActivate(ui::ICollectionView* view) override
    {
        showInAssetBrowser();
        return true;
    }

    //--

    void showInAssetBrowser() const
    {
        GetService<AssetBrowserService>()->showFile(m_depotPath);
    }

    //--

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

    //--

    void updateStatus(ImportStatus status, float time)
    {
        m_statusLabel->text(ImportStatusToDisplayText(status));

        if (time > 0)
            m_timeLabel->text(TempString("{}", TimeInterval(time)));
        else
            m_timeLabel->text("[color:#888]---[/color]");
    }

    void updateProgress(uint64_t count, uint64_t total, StringView message)
    {
    }

protected:
    ui::TextLabelPtr m_statusLabel;
    ui::TextLabelPtr m_timeLabel;

    StringBuf m_depotPath;
    StringBuf m_fileName;
};

///--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportDetailsDialog);
RTTI_END_TYPE();

AssetImportDetailsDialog::AssetImportDetailsDialog()
{
    layoutVertical();

    //--

    {
        auto leftPanel = /*split->*/createChild<ui::IElement>();
        leftPanel->layoutVertical();
        leftPanel->expand();

        auto filter = leftPanel->createChild<ui::SearchBar>();

        {
            auto bar = leftPanel->createChild<ui::ColumnHeaderBar>();
            bar->addColumn("", 30.0f, true, false, false);
            //bar->addColumn("Class", 180.0f, true, true, true);
            bar->addColumn("Status", 120.0f, true, true, false);
            bar->addColumn("Time", 90.0f, true, true, false);
            bar->addColumn("Depot path", 900.0f, false);
        }

        m_fileList = leftPanel->createChild<ui::ListViewEx>();
        m_fileList->expand();
        m_fileList->customInitialSize(800, 600);

        //m_fileList->model(m_filesListModel);
        //filter->bindItemView(m_fileList);
    }

    //--
}

AssetImportDetailsDialog::~AssetImportDetailsDialog()
{}

void AssetImportDetailsDialog::clearFiles()
{
    m_fileList->clear();
    m_fileItems.clear();
}

void AssetImportDetailsDialog::setFileStatus_AnyThread(StringBuf depotFileName, ImportStatus status, float time)
{
    if (IsMainThread())
    {
        setFileStatus_MainThread(depotFileName, status, time);
    }
    else
    {
        runSync([depotFileName, status, time, this]()
            {
                setFileStatus_MainThread(depotFileName, status, time);
            });
    }
}

void AssetImportDetailsDialog::setFileProgress_AnyThread(StringBuf depotFileName, uint64_t count, uint64_t total, StringBuf message)
{
    runSync([depotFileName, count, total, message, this]()
        {
            setFileProgress_MainThread(depotFileName, count, total, message);
        });
}

void AssetImportDetailsDialog::setFileStatus_MainThread(StringView depotFileName, ImportStatus status, float time)
{
    RefPtr<AssetImportFileDetails> item;
    if (!m_fileItems.find(depotFileName, item))
    {
        item = RefNew<AssetImportFileDetails>(depotFileName);
        m_fileItems[StringBuf(depotFileName)] = item;
        m_fileList->addItem(item);
    }

    item->updateStatus(status, time);
}

void AssetImportDetailsDialog::setFileProgress_MainThread(StringView depotFileName, uint64_t count, uint64_t total, StringView message)
{
    RefPtr<AssetImportFileDetails> item;
    if (m_fileItems.find(depotFileName, item))
        item->updateProgress(count, total, message);
}

///--

END_BOOMER_NAMESPACE_EX(ed)
