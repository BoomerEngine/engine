/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "browserService.h"

#include "editor/common/include/utils.h"

#include "core/object/include/rttiDataView.h"
#include "core/containers/include/path.h"
#include "core/resource/include/referenceType.h"
#include "core/resource/include/asyncReferenceType.h"

#include "engine/ui/include/uiDataBox.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiColorPickerBox.h"
#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiEditBox.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// resource file
class DataBoxResource : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxResource, IDataBox);

public:
    DataBoxResource(SpecificClassType<IResource> resourceClass, bool async, Type dataType)
        : m_resourceClass(resourceClass)
        , m_async(async)
        , m_dataType(dataType)
    {
        hitTest(true);
        layoutHorizontal();

        //--

        {
            m_thumbnail = createChildWithType<ui::CustomImage>("FileThumbnailBorder"_id);
        }

        //--

        {
            auto rightContainer = createChild<ui::IElement>();
            rightContainer->customVerticalAligment(ui::ElementVerticalLayout::Middle);
            rightContainer->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            rightContainer->customMargins(ui::Offsets(5, 5, 5, 5));

            {
                auto flags = { ui::EditBoxFeatureBit::AcceptsEnter };
                m_name = rightContainer->createChild<ui::EditBox>(flags);
                m_name->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                m_name->hitTest(ui::HitTestState::Enabled);
                m_name->customMargins(0, 0, 0, 4);
            }

            {
                auto buttons = rightContainer->createChild<ui::IElement>();
                buttons->layoutVertical();
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

        m_name->bind(ui::EVENT_TEXT_ACCEPTED) = [this]()
        {
            auto fileName = m_name->text();

            if (ValidateFileName(fileName))
            {
                if (m_depotPath)
                {
                    auto dirPath = m_depotPath.view().baseDirectory();
                    auto extension = m_depotPath.view().extensions();
                    changeFile(TempString("{}{}.{}", dirPath, fileName, extension));
                }
            }
        };

        /*m_thumbnail->OnClick = [this](UI_CALLBACK)
        {
            if (m_resourceClass != nullptr)
            {
                auto picker = RefNew<AssetMiniPicker>(m_resourceClass);
                if (m_currentFile)
                    picker->currentFile(m_currentFile);
                else
                    picker->currentDirectory(GetService<AssetBrowserWindow>()->selectedDirectory());

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
        m_depotPath = "";
        m_fileName = "";

        ResourceID id;
        DataViewResult ret;

        if (m_async)
        {
            ResourceAsyncRef<IResource> data;
            ret = readValue(data);
            id = data.id();
        }
        else
        {
            ResourceRef<IResource> data;
            ret = readValue(data);
            id = data.id();
        }

        if (ret.code == DataViewResultCode::ErrorManyValues)
        {
            m_thumbnail->image(""_id);
            m_thumbnail->visibility(true);

            m_name->text("<multiple values>");
            m_name->removeStyleClass("missing"_id);
            m_name->enable(false);
            m_name->tooltip("");

            m_buttonShowInBrowser->visibility(false);
            m_buttonBar->visibility(true);
        }
        else if (ret.code != DataViewResultCode::OK)
        {
            m_name->text("<error>");
            m_name->removeStyleClass("missing"_id);
            m_name->enable(false);
            m_name->tooltip("");

            m_buttonBar->visibility(false);
            m_thumbnail->visibility(false);
        }
        else
        {
            StringBuf path;
            if (GetService<DepotService>()->resolvePathForID(id, path))
            {
                // update path info
                m_depotPath = path;
                m_fileName = StringBuf(path.view().fileStem());

                // update file name
                m_name->text(m_fileName);
                m_name->tooltip(TempString("{}[br][b]{}[/b]", m_depotPath, id));
                m_name->enable(true);

                // show ui elements
                m_buttonShowInBrowser->visibility(true);
                m_buttonBar->visibility(true);
                m_thumbnail->visibility(true);
            }
            else
            {
                m_name->text("<missing asset>");
                m_name->enable(false);
                m_name->tooltip(TempString("{}", id));
                m_name->removeStyleClass("missing"_id);
            }
        }
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

    virtual ui::DragDropDataPtr queryDragDropData(const BaseKeyFlags& keys, const ui::Position& position) const override
    {
        if (m_depotPath)
            return RefNew<ed::AssetBrowserFileDragDrop>(m_depotPath);
        return nullptr;
    }

    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override
    {
        // can we handle this data ?
        auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(data);
        if (fileData && fileData->depotPath())
        {
            //if (fileData->file()->fileFormat().loadableAsType(m_resourceClass))
                return RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);
        }

        // not handled
        return nullptr;
    }

    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override
    {
        auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(data);
        if (fileData)
            changeFile(fileData->depotPath());
    }

    //--

    void DataBoxResource::changeFile(StringView depotPath)
    {
        if (depotPath)
        {
            // load the file
            if (m_async)
            {
                if (auto newRef = BuildAsyncResourceRef(depotPath, m_resourceClass))
                {
                    auto ret = writeValue(&newRef, m_dataType);
                    if (!ret.valid())
                        ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                }
                else
                {
                    ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Unable to load: '{}'", depotPath));
                }
            }
            else
            {
                if (auto loadedResourceRef = LoadResourceRef(depotPath, m_resourceClass))
                {
                    auto ret = writeValue(&loadedResourceRef, m_dataType);
                    if (!ret.valid())
                        ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
                }
                else
                {
                    ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Unable to load: '{}'", depotPath));
                }
            }
        }
        else
        {
            if (m_async)
            {
                ResourceAsyncRef<IResource> emptyRef;
                auto ret = writeValue(&emptyRef, m_dataType);
                if (!ret.valid())
                    ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
            }
            else
            {
                ResourceRef<IResource> emptyRef;
                auto ret = writeValue(&emptyRef, m_dataType);
                if (!ret.valid())
                    ui::PostWindowMessage(this, MessageType::Warning, "DataInspector"_id, TempString("Error writing value: '{}'", ret));
            }
        }
    }

    void cmdSelectCurrentAsset()
    {
        auto file = GetService<ed::AssetBrowserService>()->selectedFile();
        changeFile(file);
    }

    void cmdClearCurrentAsset()
    {
        changeFile(nullptr);
    }

    void cmdShowInAssetBrowser()
    {
        GetService<ed::AssetBrowserService>()->showFile(m_depotPath);
    }

    //--

protected:
    ui::CustomImagePtr m_thumbnail;
    ui::EditBoxPtr m_name;
    ui::ElementPtr m_buttonBar;
    ui::ElementPtr m_buttonShowInBrowser;

    SpecificClassType<IResource> m_resourceClass;
    Type m_dataType;
    bool m_async = false;

    StringBuf m_depotPath;
    StringBuf m_fileName;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxResource);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxResource");
RTTI_END_TYPE();

//--

class DataBoxResourceFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxResourceFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const DataViewInfo& info) const override
    {
        if (info.dataType->metaType() == MetaType::ResourceRef)
        {
            const auto* refType = static_cast<const ResourceRefType*>(info.dataType.ptr());
            if (const auto refClass = refType->resourceClass())
                return RefNew<DataBoxResource>(refClass, false, info.dataType);
        }
        else if (info.dataType->metaType() == MetaType::ResourceAsyncRef)
        {
            const auto* refType = static_cast<const ResourceAsyncRefType*>(info.dataType.ptr());
            if (const auto refClass = refType->resourceClass())
                return RefNew<DataBoxResource>(refClass, true, info.dataType);
        }

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxResourceFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
