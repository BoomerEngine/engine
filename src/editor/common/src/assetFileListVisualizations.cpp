/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetFileSmallImportIndicator.h"
#include "assetFileListVisualizations.h"

#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiTextValidation.h"
#include "engine/ui/include/uiEditBox.h"
#include "managedDirectoryPlaceholder.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAssetBrowserVisItem);
RTTI_END_TYPE();
    
IAssetBrowserVisItem::IAssetBrowserVisItem(AssetBrowserVisItemType type)
    : m_type(type)
{
    layoutVertical();
}

bool IAssetBrowserVisItem::handleItemFilter(const ui::SearchPattern& filter) const
{
    return filter.testString(displayName());
}

bool IAssetBrowserVisItem::handleItemSort(const ui::ICollectionItem* ptr, int colIndex) const
{
    const auto* other = static_cast<const IAssetBrowserVisItem*>(ptr);
    if (other->type() != type())
        return (int)type() < (int)other->type();

    return displayName() < other->displayName();
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAssetBrowserDepotVisItem);
RTTI_END_TYPE();

IAssetBrowserDepotVisItem::IAssetBrowserDepotVisItem(AssetBrowserVisItemType type, StringView depotPath)
    : IAssetBrowserVisItem(type)
    , m_depotPath(depotPath)
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserFileVis);
RTTI_END_TYPE();

AssetBrowserFileVis::AssetBrowserFileVis(StringView depotPath)
    : IAssetBrowserDepotVisItem(AssetBrowserVisItemType::File, depotPath)
    , m_displayName(depotPath.fileStem())
{
    hitTest(true);

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

    StringBuilder displayText;
    //file->fileFormat().printTags(displayText, "[br]");
    displayText << m_displayName;

    m_label = createChild<ui::TextLabel>(displayText.toString());
    m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);
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

AssetBrowserDirectoryVis::AssetBrowserDirectoryVis(StringView depotPath, bool parentDirectory /*= false*/)
    : IAssetBrowserDepotVisItem(parentDirectory ? AssetBrowserVisItemType::ParentDirectory : AssetBrowserVisItemType::ChildDirectory, depotPath)
    , m_displayName(parentDirectory ? ".." : depotPath.directoryName())
{
    hitTest(true);

    static const auto thumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/directory.png");

	m_icon = createChild<ui::Image>(thumbnail);
    m_icon->customMargins(3.0f);
    m_icon->customMaxSize(m_size, m_size);
    m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

    m_label = createChild<ui::TextLabel>(m_displayName);
    m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);
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

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserPlaceholderFileVis);
RTTI_END_TYPE();

AssetBrowserPlaceholderFileVis::AssetBrowserPlaceholderFileVis(const ManagedFileFormat* format, StringView parentDepotPath, StringView name)
    : IAssetBrowserVisItem(AssetBrowserVisItemType::NewFile)
    , m_parentDepotPath(parentDepotPath)
    , m_format(format)
{
    hitTest(true);

    static const auto defaultThumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/file.png");

	m_icon = createChild<ui::Image>(defaultThumbnail);
    m_icon->customMargins(3.0f);
    m_icon->customMaxSize(m_size, m_size);
    m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

    auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

    m_label = createChild<ui::EditBox>(flags);
    m_label->text(name);
    m_label->validation(ui::MakeFilenameValidationFunction());
    m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);

    auto selfRef = RefWeakPtr<AssetBrowserPlaceholderFileVis>(this);

    m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [selfRef]()
    {
        if (auto self = selfRef.lock())
        {
            const auto name = self->m_label->text();

            self->m_label->enable(false);
            self->removeSelfFromList();

            self->call(EVENT_ASSET_PLACEHOLDER_ACCEPTED, name);
        }
    };

    actions().bindShortcut("RemovePlaceholder"_id, "Escape");
    actions().bindCommand("RemovePlaceholder"_id) = [selfRef]()
    {
        if (auto self = selfRef.lock())
        {
            self->m_label->enable(false);
            self->removeSelfFromList();
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

AssetBrowserPlaceholderDirectoryVis::AssetBrowserPlaceholderDirectoryVis(StringView parentDepotPath, StringView name)
    : IAssetBrowserVisItem(AssetBrowserVisItemType::NewDirectory)
    , m_parentDepotPath(parentDepotPath)
{
    hitTest(true);

    static const auto thumbnail = LoadImageFromDepotPath("/engine/interface/thumbnails/directory.png");

	m_icon = createChild<ui::Image>(thumbnail);
    m_icon->customMargins(3.0f);
    m_icon->customMaxSize(m_size, m_size);
    m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

    auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

    m_label = createChild<ui::EditBox>(flags);
    m_label->text(name);
    m_label->validation(ui::MakeFilenameValidationFunction());
    m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
    m_label->customMargins(3.0f);

    auto selfRef = RefWeakPtr<AssetBrowserPlaceholderDirectoryVis>(this);

    m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [selfRef]()
    {
        if (auto self = selfRef.lock())
        {
            const auto name = self->m_label->text();

            self->m_label->enable(false);
            self->removeSelfFromList();

            self->call(EVENT_ASSET_PLACEHOLDER_ACCEPTED, name);
        }
    };

    actions().bindShortcut("RemovePlaceholder"_id, "Escape");
    actions().bindCommand("RemovePlaceholder"_id) = [selfRef]()
    {
        if (auto self = selfRef.lock())
        {
            self->m_label->enable(false);
            self->removeSelfFromList();
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
