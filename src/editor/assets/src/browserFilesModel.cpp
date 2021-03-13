/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "browserService.h"

#include "browserFilesModel.h"
#include "importIndicator.h"

#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiTextValidation.h"
#include "engine/ui/include/uiEditBox.h"
#include "browserFiles.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAssetBrowserVisItem);
RTTI_END_TYPE();
    
IAssetBrowserVisItem::IAssetBrowserVisItem(AssetBrowserVisItemType type, const AssetBrowserTabFilesSetup& setup)
    : m_type(type)
{
    layoutVertical();
}

void IAssetBrowserVisItem::updateIcons()
{

}

bool IAssetBrowserVisItem::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    return filter.testString(displayName());
}

void IAssetBrowserVisItem::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outData) const
{
    outData.type = (int)m_type;
    outData.index = uniqueIndex();
    outData.caption = displayName();
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAssetBrowserDepotVisItem);
RTTI_END_TYPE();

IAssetBrowserDepotVisItem::IAssetBrowserDepotVisItem(AssetBrowserVisItemType type, const AssetBrowserTabFilesSetup& setup, StringView depotPath)
    : IAssetBrowserVisItem(type, setup)
    , m_depotPath(depotPath)
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserFileVis);
RTTI_END_TYPE();

AssetBrowserFileVis::AssetBrowserFileVis(const AssetBrowserTabFilesSetup& setup, StringView depotPath)
    : IAssetBrowserDepotVisItem(AssetBrowserVisItemType::File, setup, depotPath)
    , m_displayName(depotPath.fileStem())
    , m_listItem(setup.list)
{
    hitTest(true);

    if (setup.list)
    {
        customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

        m_label = createChild<ui::TextLabel>();
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_label->customMargins(3.0f);
        m_label->tooltip(m_depotPath);
    }
    else
    {
        static const auto defaultThumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/file.png");

        m_icon = createChild<ui::Image>(defaultThumbnail);
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(m_size, m_size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        {
            m_importIndicator = m_icon->createChild<AssetFileSmallImportIndicator>(depotPath);
            m_importIndicator->customMargins(20, 20, 0, 0);
            m_importIndicator->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
            m_importIndicator->customVerticalAligment(ui::ElementVerticalLayout::Top);
            m_importIndicator->visibility(m_size >= 80);
            m_importIndicator->overlay(true);
        }

        m_label = createChild<ui::TextLabel>();
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);
        m_label->tooltip(m_depotPath);
    }

    updateIcons();
}

void AssetBrowserFileVis::updateIcons()
{
    int mark = 0;
    GetService<AssetBrowserService>()->checkFileMark(m_depotPath, mark);

    StringBuilder txt;

    // file name
    if (m_listItem)
    {
        txt << "[img:page] ";
        txt << m_displayName;
    }
    else
    {
        //file->fileFormat().printTags(displayText, "[br]");
        txt << m_displayName;
    }

    // file mark
    if (mark == FILE_MARK_CUT)
    {
        txt << "[tag:#DDA][b][img:cut] [color:#000]Cut[/color][/b][/tag]";
    }
    else if (mark == FILE_MARK_COPY)
    {
        txt << "[tag:#ADA][b][img:copy] [color:#000]Copy[/color][/b][/tag]";
    }

    m_label->text(txt.view());

    // make icon half transparent when cutting
    if (m_icon)
        m_icon->customStyle<float>("opacity"_id, (mark == FILE_MARK_CUT) ? 0.5f : 1.0f);
}

void AssetBrowserFileVis::resizeIcon(uint32_t size)
{
    if (m_size != size)
    {
        if (m_icon)
            m_icon->customMaxSize(size, size);
        if (m_importIndicator)
            m_importIndicator->visibility(size >= 80);
        m_size = size;
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserDirectoryVis);
RTTI_END_TYPE();

AssetBrowserDirectoryVis::AssetBrowserDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView depotPath)
    : IAssetBrowserDepotVisItem(AssetBrowserVisItemType::ChildDirectory, setup, depotPath)
    , m_displayName(depotPath.directoryName())
    , m_listItem(setup.list)
{
    hitTest(true);

    if (setup.list)
    {
        customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

        m_label = createChild<ui::TextLabel>();
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_label->customMargins(3.0f);
    }
    else
    {
        static const auto thumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/directory.png");

        m_icon = createChild<ui::Image>(thumbnail);
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(m_size, m_size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        m_label = createChild<ui::TextLabel>();
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);
    }


    updateLabel();
}
    
void AssetBrowserDirectoryVis::resizeIcon(uint32_t size)
{
    if (m_size != size)
    {
        if (m_icon)
            m_icon->customMaxSize(size, size);
        m_size = size;
    }
}

void AssetBrowserDirectoryVis::updateLabel()
{
    StringBuilder txt;

    if (m_listItem)
        txt << "[img:folder] ";

    txt << m_displayName;

    if (GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath))
        txt << " [img:star]";

    m_label->text(txt.view());
}

void AssetBrowserDirectoryVis::updateIcons()
{
    updateLabel();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserParentDirectoryVis);
RTTI_END_TYPE();

AssetBrowserParentDirectoryVis::AssetBrowserParentDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView depotPath)
    : IAssetBrowserDepotVisItem(AssetBrowserVisItemType::ParentDirectory, setup, depotPath)
{
    hitTest(true);

    if (setup.list)
    {
        customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

        m_label = createChild<ui::TextLabel>("[img:arrow_short_up_green] ..");
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_label->customMargins(3.0f);
    }
    else
    {
        static const auto thumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/directory.png");

        m_icon = createChild<ui::Image>(thumbnail);
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(m_size, m_size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        m_label = createChild<ui::TextLabel>("..");
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);
    }
}

void AssetBrowserParentDirectoryVis::resizeIcon(uint32_t size)
{
    if (m_size != size)
    {
        if (m_icon)
            m_icon->customMaxSize(size, size);
        m_size = size;
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserPlaceholderFileVis);
RTTI_END_TYPE();

AssetBrowserPlaceholderFileVis::AssetBrowserPlaceholderFileVis(const AssetBrowserTabFilesSetup& setup, ResourceClass cls, StringView parentDepotPath, StringView name)
    : IAssetBrowserVisItem(AssetBrowserVisItemType::NewFile, setup)
    , m_parentDepotPath(parentDepotPath)
    , m_resourceClass(cls)
{
    hitTest(true);

    if (setup.list)
    {
        layoutHorizontal();
        customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        createChild<ui::TextLabel>("[img:page_new] ");
    }
    else
    {
        static const auto defaultThumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/file.png");

        m_icon = createChild<ui::Image>(defaultThumbnail);
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(m_size, m_size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    }

    auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

    m_label = createChild<ui::EditBox>(flags);
    m_label->text(name);
    m_label->validation(ui::MakeFilenameValidationFunction());
    m_label->customHorizontalAligment(setup.list ? ui::ElementHorizontalLayout::Expand : ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);

    auto self = RefWeakPtr<AssetBrowserPlaceholderFileVis>(this);

    m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [self]()
    {
        if (auto ptr = self.lock())
        {
            const auto name = ptr->m_label->text();

            ptr->m_label->enable(false);
            ptr->removeSelfFromList();

            ptr->call(EVENT_ASSET_PLACEHOLDER_ACCEPTED, name);
        }
    };

    bindShortcut("Escape") = [self]()
    {
        if (auto ptr = self.lock())
        {
            ptr->m_label->enable(false);
            ptr->removeSelfFromList();
        }
    };
}

void AssetBrowserPlaceholderFileVis::resizeIcon(uint32_t size)
{
    if (m_size != size)
    {
        if (m_icon)
            m_icon->customMaxSize(size, size);
        m_size = size;
    }
}

StringView AssetBrowserPlaceholderFileVis::displayName() const
{
    return "";
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserPlaceholderDirectoryVis);
RTTI_END_TYPE();

AssetBrowserPlaceholderDirectoryVis::AssetBrowserPlaceholderDirectoryVis(const AssetBrowserTabFilesSetup& setup, StringView parentDepotPath, StringView name)
    : IAssetBrowserVisItem(AssetBrowserVisItemType::NewDirectory, setup)
    , m_parentDepotPath(parentDepotPath)
{
    hitTest(true);

    if (setup.list)
    {
        layoutHorizontal();
        customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        createChild<ui::TextLabel>("[img:folder_new] ");
    }
    else
    {
        static const auto thumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/directory.png");

        m_icon = createChild<ui::Image>(thumbnail);
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(m_size, m_size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    }

    auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

    m_label = createChild<ui::EditBox>(flags);
    m_label->text(name);
    m_label->validation(ui::MakeFilenameValidationFunction());
    m_label->customHorizontalAligment(setup.list ? ui::ElementHorizontalLayout::Expand : ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);

    auto self = RefWeakPtr<AssetBrowserPlaceholderDirectoryVis>(this);

    m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [self]()
    {
        if (auto ptr = self.lock())
        {
            const auto name = ptr->m_label->text();

            ptr->m_label->enable(false);
            ptr->removeSelfFromList();

            ptr->call(EVENT_ASSET_PLACEHOLDER_ACCEPTED, name);
        }
    };

    bindShortcut("Escape") = [self]()
    {
        if (auto ptr = self.lock())
        {
            ptr->m_label->enable(false);
            ptr->removeSelfFromList();
        }
    };
}

void AssetBrowserPlaceholderDirectoryVis::resizeIcon(uint32_t size)
{
    if (m_size != size)
    {
        if (m_icon)
            m_icon->customMaxSize(size, size);
        m_size = size;
    }
}

StringView AssetBrowserPlaceholderDirectoryVis::displayName() const
{
    return "";
}

//--

END_BOOMER_NAMESPACE_EX(ed)
