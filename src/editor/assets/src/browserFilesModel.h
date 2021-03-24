/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiListViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetFileSmallImportIndicator;
struct AssetBrowserTabFilesSetup;

//--

enum class AssetBrowserVisItemType : uint8_t
{
    Invalid,
    ParentDirectory,
    ChildDirectory,
    File,
    NewFile,
    NewDirectory,
};

//--

// general visualization of something in the asset browser file list
class IAssetBrowserVisItem : public ui::IListItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(IAssetBrowserVisItem, ui::IListItem);

public:
    IAssetBrowserVisItem(AssetBrowserVisItemType type, const AssetBrowserTabFilesSetup& setup);

    INLINE AssetBrowserVisItemType type() const { return m_type; }

    virtual StringView displayName() const = 0;

    virtual void updateIcons();

    virtual void resizeIcon(uint32_t size) = 0;

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outData) const override;

protected:
    AssetBrowserVisItemType m_type;
    uint32_t m_size = 100;
};

//--

// general visualization element that has a depot path
class IAssetBrowserDepotVisItem : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(IAssetBrowserDepotVisItem, IAssetBrowserVisItem);

public:
    IAssetBrowserDepotVisItem(AssetBrowserVisItemType type, const AssetBrowserTabFilesSetup& setup, StringView depotPath);

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

protected:
    StringBuf m_depotPath;
};

//--

// general visualization element for normal file
class AssetBrowserFileVis : public IAssetBrowserDepotVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserFileVis, IAssetBrowserDepotVisItem);

public:
    AssetBrowserFileVis(const AssetBrowserTabFilesSetup& setup, StringView depotPath);

    virtual StringView displayName() const override final { return m_displayName; }

    virtual void updateIcons();

protected:
    StringBuf m_displayName;

    ui::CustomImagePtr m_icon;
    ui::TextLabelPtr m_label;
    bool m_listItem = false;

    RefPtr<AssetFileSmallImportIndicator> m_importIndicator;

    virtual void resizeIcon(uint32_t size) override;
};

//--

// general visualization element for normal directory
class AssetBrowserDirectoryVis : public IAssetBrowserDepotVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirectoryVis, IAssetBrowserDepotVisItem);

public:
    AssetBrowserDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView depotPath);

    virtual StringView displayName() const override final { return m_displayName; }

    virtual void resizeIcon(uint32_t size) override;

    virtual void updateIcons();

protected:
    StringBuf m_displayName;
    bool m_listItem = false;

    ui::CustomImagePtr m_icon;
    ui::TextLabelPtr m_label;

    void updateLabel();
};

//--

// special visualization element for parent directory
class AssetBrowserParentDirectoryVis : public IAssetBrowserDepotVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserParentDirectoryVis, IAssetBrowserDepotVisItem);

public:
    AssetBrowserParentDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView parentDepotPath);

    virtual StringView displayName() const override final { return ".."; }

    virtual void resizeIcon(uint32_t size) override;

protected:
    ui::CustomImagePtr m_icon;
    ui::TextLabelPtr m_label;
};

//--

DECLARE_UI_EVENT(EVENT_ASSET_PLACEHOLDER_ACCEPTED, StringBuf);

//--

// general visualization element for PLACEHOLDER FILE (file being created)
class AssetBrowserPlaceholderFileVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserPlaceholderFileVis, IAssetBrowserVisItem);

public:
    AssetBrowserPlaceholderFileVis(const AssetBrowserTabFilesSetup& setup, ResourceClass cls, StringView parentDepotPath, StringView name);

    INLINE const StringBuf& parentDepotPath() const { return m_parentDepotPath; }
    INLINE const ResourceClass resourceClass() const { return m_resourceClass; }

    virtual StringView displayName() const override final;

    virtual void resizeIcon(uint32_t size) override;

protected:
    StringBuf m_parentDepotPath;
    ResourceClass m_resourceClass;

    ui::CustomImagePtr m_icon;
    ui::EditBoxPtr m_label;
};

//--

// general visualization element for PLACEHOLDER DIRECTORY (dir being created)
class AssetBrowserPlaceholderDirectoryVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserPlaceholderDirectoryVis, IAssetBrowserVisItem);

public:
    AssetBrowserPlaceholderDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView parentDepotPath, StringView name);

    INLINE const StringBuf& parentDepotPath() const { return m_parentDepotPath; }

    virtual StringView displayName() const override final;

    virtual void resizeIcon(uint32_t size) override;

protected:
    StringBuf m_parentDepotPath;

    ui::CustomImagePtr m_icon;
    ui::EditBoxPtr m_label;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
