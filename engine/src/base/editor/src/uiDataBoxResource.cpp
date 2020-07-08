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

namespace ui
{

    //--

    /// resource file
    class DataBoxResource : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxResource, IDataBox);

    public:
        DataBoxResource(base::SpecificClassType<base::res::IResource> resourceClass)
            : m_resourceClass(resourceClass)
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
                        button->OnClick = [this]()
                        {
                            cmdSelectCurrentAsset();
                        };
                    }

                    {
                        auto button = buttons->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:cross][size:-]Clear");
                        button->tooltip("Remove resource reference");
                        button->OnClick = [this]()
                        {
                            cmdClearCurrentAsset();
                        };
                    }

                    {
                        auto button = buttons->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:zoom][size:-]Show");
                        button->tooltip("Show selected resource in asset browser");
                        button->OnClick = [this]()
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
                    auto picker = base::CreateSharedPtr<AssetMiniPicker>(m_resourceClass);
                    if (m_currentFile)
                        picker->currentFile(m_currentFile);
                    else
                        picker->currentDirectory(base::GetService<AssetBrowser>()->selectedDirectory());

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
            base::StringView<char> fileName = "Invalid";
            base::StringView<char> filePath = "";
            bool fileFound = false;

            base::res::Ref<base::res::IResource> data;
            const auto ret = readValue(data);
            if (ret.code == base::DataViewResultCode::OK)
            {
                const auto path = data.key().path().path();

                auto managedFile = base::GetService<ed::Editor>()->managedDepot().findManagedFile(path);
                if (managedFile != m_currentFile)
                {
                    base::image::ImageRef imageRef;
                    if (managedFile)
                        imageRef = managedFile->typeThumbnail();
                    m_thumbnail->image(imageRef);
                    m_currentFile = managedFile;
                }

                // set file name
                if (!path.empty())
                {
                    fileName = path.afterLast("/");
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
            else if (ret.code == base::DataViewResultCode::ErrorManyValues)
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
                m_name->text(base::TempString("[tag:#F00][img:error] {}[/tag]", ret));
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
            auto fileData = base::rtti_cast<ed::AssetBrowserFileDragDrop>(data);
            if (fileData && fileData->file())
            {
                if (fileData->file()->fileFormat().loadableAsType(m_resourceClass))
                    return base::CreateSharedPtr<ui::DragDropHandlerGeneric>(data, this, entryPosition);
            }

            // not handled
            return nullptr;
        }

        void DataBoxResource::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            auto fileData = base::rtti_cast<ed::AssetBrowserFileDragDrop>(data);
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
                    auto resPath = base::res::ResourcePath(newFile->depotPath().c_str());
                    if (!resPath.empty())
                    {
                        // load the file
                        if (auto loadedResource = base::LoadResource(base::res::ResourceKey(resPath, m_resourceClass)))
                        {
                            const auto dataType = base::reflection::GetTypeObject<base::res::Ref<base::res::IResource>>();
                            if (const auto ret = HasError(writeValue(&loadedResource, dataType)))
                                ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, base::TempString("Error writing value: '{}'", ret));
                        }
                        else
                        {
                            ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, base::TempString("Unable to load '{}'.[br]Target class '{}'", resPath, m_resourceClass));
                        }
                    }
                }
                else
                {
                    base::res::Ref<base::res::IResource> emptyRef;
                    const auto dataType = base::reflection::GetTypeObject<base::res::Ref<base::res::IResource>>();

                    if (const auto ret = HasError(writeValue(&emptyRef, dataType)))
                        ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, base::TempString("Error writing value: '{}'", ret));
                }
            }
        }

        void cmdSelectCurrentAsset()
        {
            auto file = base::GetService<ed::Editor>()->selectedFile();
            changeFile(file);
        }

        void cmdClearCurrentAsset()
        {
            changeFile(nullptr);
        }

        void cmdShowInAssetBrowser()
        {
            if (m_currentFile)
                base::GetService<ed::Editor>()->selectFile(m_currentFile);
        }

    protected:
        ui::ImagePtr m_thumbnail;
        ui::TextLabelPtr m_name;
        ui::ElementPtr m_buttonBar;
        ui::ElementPtr m_buttonShowInBrowser;

        base::SpecificClassType<base::res::IResource> m_resourceClass;

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
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            if (info.dataType->metaType() == base::rtti::MetaType::ResourceRef)
            {
                const auto* refType = static_cast<const base::res::ResourceRefType*>(info.dataType.ptr());
                if (const auto refClass = refType->resourceClass())
                    return base::CreateSharedPtr<DataBoxResource>(refClass);
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxResourceFactory);
    RTTI_END_TYPE();

    //--

} // ui