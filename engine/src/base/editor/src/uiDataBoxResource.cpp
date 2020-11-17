/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "base/ui/include/uiDataBox.h"
#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiColorPickerBox.h"
#include "base/ui/include/uiImage.h"

#include "base/object/include/rttiDataView.h"
#include "base/resource/include/resourceReferenceType.h"
#include "editorService.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "assetBrowser.h"
#include "base/resource/include/resourceAsyncReferenceType.h"

namespace ui
{

    //--

    /// resource file
    class DataBoxResource : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxResource, IDataBox);

    public:
        DataBoxResource(SpecificClassType<res::IResource> resourceClass, bool async, Type dataType)
            : m_resourceClass(resourceClass)
            , m_async(async)
            , m_dataType(dataType)
        {
            hitTest(true);
            layoutHorizontal();

            //--

            {
                m_thumbnail = createChildWithType<ui::Image>("FileThumbnailBorder"_id);
            }

            //--

            {
                auto rightContainer = createChild<ui::IElement>();
                rightContainer->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                rightContainer->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                rightContainer->customMargins(ui::Offsets(5, 5, 5, 5));

                {
                    m_name = rightContainer->createChild<ui::TextLabel>();
                    m_name->hitTest(ui::HitTestState::Enabled);
                    m_name->customMargins(0, 0, 0, 4);
                }

                {
                    auto buttons = rightContainer->createChild<ui::IElement>();
                    buttons->layoutHorizontal();
                    m_buttonBar = buttons;

                    {
                        auto button = buttons->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:goto_prev][size:-]Use");
                        button->tooltip("Use resource from asset browser");
                        button->bind(EVENT_CLICKED) = [this]()
                        {
                            cmdSelectCurrentAsset();
                        };
                    }

                    {
                        auto button = buttons->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:cross][size:-]Clear");
                        button->tooltip("Remove resource reference");
                        button->bind(EVENT_CLICKED) = [this]()
                        {
                            cmdClearCurrentAsset();
                        };
                    }

                    {
                        auto button = buttons->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:zoom][size:-]Show");
                        button->tooltip("Show selected resource in asset browser");
                        button->bind(EVENT_CLICKED) = [this]()
                        {
                            cmdShowInAssetBrowser();
                        };

                        m_buttonShowInBrowser = button;
                    }
                }
            }

            /*m_thumbnail->OnClick = [this](UI_CALLBACK)
            {
                if (m_resourceClass != nullptr)
                {
                    auto picker = CreateSharedPtr<AssetMiniPicker>(m_resourceClass);
                    if (m_currentFile)
                        picker->currentFile(m_currentFile);
                    else
                        picker->currentDirectory(GetService<AssetBrowser>()->selectedDirectory());

                    picker->OnFileSelected = [this, picker](UI_CALLBACK)
                    {
                        auto newFile = picker->selectedFile();
                        changeFile(newFile);
                    };

                    AttachOverlay(sharedFromThis(), picker, ui::OverlayAnchorMode::RelativeBottomLeft);
                    picker->focusFileList();
                }
            };*/
        }

        virtual void handleValueChange() override
        {
            StringView fileName = "Invalid";
            StringView filePath = "";
            bool fileFound = false;

            res::ResourceKey key;
            DataViewResult ret;

            if (m_async)
            {
                res::AsyncRef<res::IResource> data;
                ret = readValue(data);
                key = data.key();
            }
            else
            {
                res::Ref<res::IResource> data;
                ret = readValue(data);
                key = data.key();
            }

            if (ret.code == DataViewResultCode::OK)
            {
                auto managedFile = GetService<ed::Editor>()->managedDepot().findManagedFile(key.path());
                if (managedFile != m_currentFile)
                {
                    image::ImageRef imageRef;
                    if (managedFile)
                        imageRef = managedFile->typeThumbnail();
                    m_thumbnail->image(imageRef);
                    m_currentFile = managedFile;
                }

                // set file name
                if (key)
                {
                    fileName = key.fileName();
                    fileFound = (managedFile != nullptr);
                }
                else
                {
                    fileName = "None";
                    fileFound = true;
                }

                // update file name
                m_name->text(fileName);
                m_name->tooltip(filePath);

                // show ui elements
                m_buttonShowInBrowser->visibility(true);
                m_buttonBar->visibility(true);
                m_thumbnail->visibility(true);
            }
            else if (ret.code == DataViewResultCode::ErrorManyValues)
            {
                m_thumbnail->image(nullptr);
                m_thumbnail->visibility(true);

                m_name->text("<multiple values>");
                m_name->tooltip("");

                m_buttonShowInBrowser->visibility(false);
                m_buttonBar->visibility(true);
            }
            else
            {
                m_name->text(TempString("[tag:#F00][img:error] {}[/tag]", ret));
                m_name->tooltip("");

                m_buttonBar->visibility(false);
                m_thumbnail->visibility(false);
            }

            /*if (fileFound)
                m_name->removeStyleClass("missing");
            else
                m_name->addStyleClass("missing");*/
        }

        virtual bool canExpandChildren() const override
        {
            return false;
        }

        virtual void enterEdit() override
        {
        }

        virtual void cancelEdit() override
        {

        }

        //--

        ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            // can we handle this data ?
            auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(data);
            if (fileData && fileData->file())
            {
                if (fileData->file()->fileFormat().loadableAsType(m_resourceClass))
                    return CreateSharedPtr<ui::DragDropHandlerGeneric>(data, this, entryPosition);
            }

            // not handled
            return nullptr;
        }

        void DataBoxResource::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(data);
            if (fileData)
                changeFile(fileData->file());
        }

        //--

        void DataBoxResource::changeFile(ed::ManagedFile* newFile)
        {
            if (m_currentFile != newFile)
            {
                // get depot path to the file
                if (newFile)
                {
                    auto resPath = newFile->depotPath();
                    if (!resPath.empty())
                    {
                        const auto key = res::ResourceKey(resPath, m_resourceClass);

                        // load the file
                        if (m_async)
                        {
                            res::AsyncRef<res::IResource> newRef = key;
                            if (const auto ret = HasError(writeValue(&newRef, m_dataType)))
                                ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                        }
                        else if (auto loadedResource = LoadResource(key))
                        {
                            if (const auto ret = HasError(writeValue(&loadedResource, m_dataType)))
                                ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                        }
                        else
                        {
                            ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Unable to load '{}'.[br]Target class '{}'", resPath, m_resourceClass));
                        }
                    }
                }
                else
                {
                    if (m_async)
                    {
                        res::AsyncRef<res::IResource> emptyRef;
                        if (const auto ret = HasError(writeValue(&emptyRef, m_dataType)))
                            ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                    }
                    else
                    {
                        res::Ref<res::IResource> emptyRef;
                        if (const auto ret = HasError(writeValue(&emptyRef, m_dataType)))
                            ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                    }
                }
            }
        }

        void cmdSelectCurrentAsset()
        {
            auto file = GetService<ed::Editor>()->mainWindow().selectedFile();
            changeFile(file);
        }

        void cmdClearCurrentAsset()
        {
            changeFile(nullptr);
        }

        void cmdShowInAssetBrowser()
        {
            if (m_currentFile)
                GetService<ed::Editor>()->mainWindow().selectFile(m_currentFile);
        }

        //--

    protected:
        ui::ImagePtr m_thumbnail;
        ui::TextLabelPtr m_name;
        ui::ElementPtr m_buttonBar;
        ui::ElementPtr m_buttonShowInBrowser;

        SpecificClassType<res::IResource> m_resourceClass;
        Type m_dataType;
        bool m_async = false;

        ed::ManagedFile* m_currentFile = nullptr;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxResource);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxResource");
    RTTI_END_TYPE();

    //--

    class DataBoxResourceFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxResourceFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const rtti::DataViewInfo& info) const override
        {
            if (info.dataType->metaType() == rtti::MetaType::ResourceRef)
            {
                const auto* refType = static_cast<const res::ResourceRefType*>(info.dataType.ptr());
                if (const auto refClass = refType->resourceClass())
                    return CreateSharedPtr<DataBoxResource>(refClass, false, info.dataType);
            }
            else if (info.dataType->metaType() == rtti::MetaType::AsyncResourceRef)
            {
                const auto* refType = static_cast<const res::ResourceAsyncRefType*>(info.dataType.ptr());
                if (const auto refClass = refType->resourceClass())
                    return CreateSharedPtr<DataBoxResource>(refClass, true, info.dataType);
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxResourceFactory);
    RTTI_END_TYPE();

    //--

} // ui