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

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedFileNativeResource.h"
#include "managedFilePlaceholder.h"

#include "base/ui/include/uiImage.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiTextValidation.h"
#include "base/ui/include/uiEditBox.h"
#include "managedDirectoryPlaceholder.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAssetBrowserVisItem);
    RTTI_END_TYPE();
    
    IAssetBrowserVisItem::IAssetBrowserVisItem(ManagedItem* item, uint32_t size)
        : m_item(item)
        , m_size(size)
    {}

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserFileVis);
    RTTI_END_TYPE();

    AssetBrowserFileVis::AssetBrowserFileVis(ManagedFile* file, uint32_t size)
        : IAssetBrowserVisItem(file, size)
        , m_file(file)
    {
        hitTest(true);

		m_icon = createChild<ui::Image>();// file->typeThumbnail());
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(size, size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
        {
            m_importIndicator = m_icon->createChild<AssetFileSmallImportIndicator>(nativeFile);
            m_importIndicator->customMargins(20, 20, 0, 0);
            m_importIndicator->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
            m_importIndicator->customVerticalAligment(ui::ElementVerticalLayout::Top);
            m_importIndicator->visibility(size >= 80);
            m_importIndicator->overlay(true);
        }

        StringBuilder displayText;
        for (const auto& tag : file->fileFormat().tags())
        {
            displayText.appendf("[tag:{}]", tag.color);

            if (tag.baked)
                displayText.appendf("[img:cog] ");
            else
                displayText.appendf("[img:file_empty_edit] ");

            if (tag.color.luminanceSRGB() > 0.5f)
            {
                displayText << "[color:#000]";
                displayText << tag.name;
                displayText << "[/color]";
            }
            else
            {
                displayText << tag.name;
            }

            displayText << "[/tag]";
            displayText << "[br]";
        }

        displayText << file->name().stringBeforeLast(".");

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

    AssetBrowserDirectoryVis::AssetBrowserDirectoryVis(ManagedDirectory* dir, uint32_t size, bool parentDirectory /*= false*/)
        : IAssetBrowserVisItem(dir, size)
        , m_directory(dir)
    {
        hitTest(true);

		m_icon = createChild<ui::Image>();// dir->typeThumbnail());
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(size, size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        m_label = createChild<ui::TextLabel>(parentDirectory ? ".." : dir->name());
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

    AssetBrowserPlaceholderFileVis::AssetBrowserPlaceholderFileVis(ManagedFilePlaceholder* placeholder, uint32_t size)
        : IAssetBrowserVisItem(placeholder, size)
        , m_filePlaceholder(AddRef(placeholder))
    {
        hitTest(true);

		m_icon = createChild<ui::Image>();// placeholder->typeThumbnail());
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(size, size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

        m_label = createChild<ui::EditBox>(flags);
        m_label->text(placeholder->shortName());
        m_label->validation(ui::MakeFilenameValidationFunction());
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);

        auto selfRef = base::RefWeakPtr<AssetBrowserPlaceholderFileVis>(this);

        m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [selfRef]()
        {
            if (auto self = selfRef.lock())
            {
                self->m_filePlaceholder->rename(self->m_label->text());
                self->m_label->enable(false);
                base::DispatchGlobalEvent(self->m_filePlaceholder->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED);
                base::DispatchGlobalEvent(self->m_filePlaceholder->depot()->eventKey(), EVENT_MANAGED_DEPOT_PLACEHOLDER_ACCEPTED, ManagedItemPtr(AddRef(self->item())));
            }
        };

        actions().bindShortcut("RemovePlaceholder"_id, "Escape");
        actions().bindCommand("RemovePlaceholder"_id) = [selfRef]()
        {
            if (auto self = selfRef.lock())
            {
                self->m_label->enable(false);
                base::DispatchGlobalEvent(self->m_filePlaceholder->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED);
                base::DispatchGlobalEvent(self->m_filePlaceholder->depot()->eventKey(), EVENT_MANAGED_DEPOT_PLACEHOLDER_DISCARDED, ManagedItemPtr(AddRef(self->item())));
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

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserPlaceholderDirectoryVis);
    RTTI_END_TYPE();

    AssetBrowserPlaceholderDirectoryVis::AssetBrowserPlaceholderDirectoryVis(ManagedDirectoryPlaceholder* placeholder, uint32_t size)
        : IAssetBrowserVisItem(placeholder, size)
        , m_directoryPlaceholder(AddRef(placeholder))
    {
        hitTest(true);

		m_icon = createChild<ui::Image>();// placeholder->typeThumbnail());
        m_icon->customMargins(3.0f);
        m_icon->customMaxSize(size, size);
        m_icon->customHorizontalAligment(ui::ElementHorizontalLayout::Center);

        auto flags = ui::EditBoxFeatureBit::AcceptsEnter;

        m_label = createChild<ui::EditBox>(flags);
        m_label->text(placeholder->name());
        m_label->validation(ui::MakeFilenameValidationFunction());
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        m_label->customMargins(3.0f);

        auto selfRef = base::RefWeakPtr<AssetBrowserPlaceholderDirectoryVis>(this);

        m_label->bind(ui::EVENT_TEXT_ACCEPTED) = [selfRef]()
        {
            if (auto self = selfRef.lock())
            {
                self->m_directoryPlaceholder->rename(self->m_label->text());
                self->m_label->enable(false);
                base::DispatchGlobalEvent(self->m_directoryPlaceholder->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED);
                base::DispatchGlobalEvent(self->m_directoryPlaceholder->depot()->eventKey(), EVENT_MANAGED_DEPOT_PLACEHOLDER_ACCEPTED, ManagedItemPtr(AddRef(self->item())));
            }
        };

        actions().bindShortcut("RemovePlaceholder"_id, "Escape");
        actions().bindCommand("RemovePlaceholder"_id) = [selfRef]()
        {
            if (auto self = selfRef.lock())
            {
                self->m_label->enable(false);
                base::DispatchGlobalEvent(self->m_directoryPlaceholder->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED);
                base::DispatchGlobalEvent(self->m_directoryPlaceholder->depot()->eventKey(), EVENT_MANAGED_DEPOT_PLACEHOLDER_DISCARDED, ManagedItemPtr(AddRef(self->item())));
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


    //--

} // ed