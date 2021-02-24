/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "base/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

class AssetFileSmallImportIndicator;

// general visualization element for normal file or directory
class IAssetBrowserVisItem : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IAssetBrowserVisItem, ui::IElement);

public:
    IAssetBrowserVisItem(ManagedItem* item, uint32_t size);

    INLINE ManagedItem* item() const { return m_item; }

    virtual void resizeIcon(uint32_t size) = 0;

protected:
    ManagedItem* m_item = nullptr;
    uint32_t m_size = 100;
};

//--

// general visualization element for normal file
class AssetBrowserFileVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserFileVis, IAssetBrowserVisItem);

public:
    AssetBrowserFileVis(ManagedFile* item, uint32_t size);

    INLINE ManagedFile* file() const { return m_file; }

    virtual void resizeIcon(uint32_t size) override;

protected:
    ManagedFile* m_file = nullptr;

    ui::ImagePtr m_icon;
    ui::TextLabelPtr m_label;

    RefPtr<AssetFileSmallImportIndicator> m_importIndicator;
};

//--

// general visualization element for normal directory
class AssetBrowserDirectoryVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirectoryVis, IAssetBrowserVisItem);

public:
    AssetBrowserDirectoryVis(ManagedDirectory* dir, uint32_t size, bool parentDirectory = false);

    INLINE ManagedDirectory* directory() const { return m_directory; }

    virtual void resizeIcon(uint32_t size) override;

protected:
    ManagedDirectory* m_directory = nullptr;

    ui::ImagePtr m_icon;
    ui::TextLabelPtr m_label;
};

//--

// general visualization element for PLACEHOLDER FILE (file being created)
class AssetBrowserPlaceholderFileVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserPlaceholderFileVis, IAssetBrowserVisItem);

public:
    AssetBrowserPlaceholderFileVis(ManagedFilePlaceholder* file, uint32_t size);

    INLINE ManagedFilePlaceholder* filePlaceholder() const { return m_filePlaceholder; }

    virtual void resizeIcon(uint32_t size) override;

protected:
    ManagedFilePlaceholderPtr m_filePlaceholder;

    ui::ImagePtr m_icon;
    ui::EditBoxPtr m_label;
};

//--

// general visualization element for PLACEHOLDER DIRECTORY (dir being created)
class AssetBrowserPlaceholderDirectoryVis : public IAssetBrowserVisItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserPlaceholderDirectoryVis, IAssetBrowserVisItem);

public:
    AssetBrowserPlaceholderDirectoryVis(ManagedDirectoryPlaceholder* dir, uint32_t size);

    INLINE ManagedDirectoryPlaceholder* filePlaceholder() const { return m_directoryPlaceholder; }

    virtual void resizeIcon(uint32_t size) override;

protected:
    ManagedDirectoryPlaceholderPtr m_directoryPlaceholder;

    ui::ImagePtr m_icon;
    ui::EditBoxPtr m_label;
};

//--

END_BOOMER_NAMESPACE(ed)