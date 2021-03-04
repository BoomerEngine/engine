/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "previewPanel.h"
#include "materialsPanel.h"

#include "engine/mesh/include/mesh.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"

#include "editor/common/include/assetBrowser.h"
#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileFormat.h"

#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiListView.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiDragDrop.h"
#include "engine/ui/include/uiCheckBox.h"

#include "core/resource/include/path.h"
#include "core/resource/include/loadingService.h"
#include "core/object/include/dataViewNative.h"
#include "core/object/include/rttiDataView.h"
#include "engine/ui/include/uiToolBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshMaterialParameters);
RTTI_END_TYPE();

MeshMaterialParameters::MeshMaterialParameters(MaterialInstance* data, StringID name)
    : m_name(name)
    , m_data(AddRef(data))
{
    updateDisplayString();
}

bool MeshMaterialParameters::updateDisplayString()
{
    StringBuilder txt;

    txt.append("[img:material] ");
    txt.append(m_name);

    if (m_data)
    {
        if (auto materialTemplate = m_data->resolveTemplate())
        {
            if (const auto fileName = materialTemplate->loadPath().view().fileStem())
            {
                txt.appendf(" [tag:#888]{}[/tag]", fileName);
            }

            bool hasTextureOverrides = false;
            bool hasParamOverrides = false;

            for (const auto& param : materialTemplate->parameters())
            {
                if (m_data->checkParameterOverride(param->name()))
                {
                    const auto metaType = param->queryDataType().metaType();
                    if (metaType == MetaType::ResourceRef || metaType == MetaType::ResourceAsyncRef)
                        hasTextureOverrides = true;
                    else
                        hasParamOverrides = true;

                    if (hasParamOverrides && hasTextureOverrides)
                        break;
                }
            }

            if (hasTextureOverrides)
                txt.append("  [tag:#A55]Changed Textures[/tag]");

            if (hasParamOverrides)
                txt.append("  [tag:#5A5]Changed Parameters[/tag]");
        }
    }

    auto newCaption = txt.toString();
    if (newCaption != m_displayString)
    {
        m_displayString = newCaption;
        return true;
    }

    return false;
}

bool MeshMaterialParameters::baseMaterial(const MaterialRef& material)
{
    if (m_data)
    {
        m_data->baseMaterial(material);
        return true;
    }

    return false;
}

//---

MeshMaterialListModel::MeshMaterialListModel()
{}

StringID MeshMaterialListModel::materialName(const ui::ModelIndex& index) const
{
    if (auto elem = data(index))
        return elem->name();
    return StringID::EMPTY();
}

ui::ModelIndex MeshMaterialListModel::findMaterial(StringID name) const
{
    for (uint32_t i = 0; i < size(); ++i)
    {
        const auto& elem = at(i);
        if (elem->name() == name)
            return index(elem);
    }

    return ui::ModelIndex();
}

bool MeshMaterialListModel::compare(const RefPtr<MeshMaterialParameters>& a, const RefPtr<MeshMaterialParameters>& b, int colIndex) const
{
    return a->name().view() < b->name().view();
}

bool MeshMaterialListModel::filter(const RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
{
    return filter.testString(data->name().view());
}

StringBuf MeshMaterialListModel::content(const RefPtr<MeshMaterialParameters>& data, int colIndex /*= 0*/) const
{
    return data->displayString();
}

ui::DragDropHandlerPtr MeshMaterialListModel::handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& dragData, const ui::Position& pos)
{
    if (auto elem = data(item))
    {
        // can we handle this data ?
        auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(dragData);
        if (fileData && fileData->file())
        {
            if (fileData->file()->fileFormat().loadableAsType(IMaterial::GetStaticClass()))
                return RefNew<ui::DragDropHandlerGeneric>(dragData, view, pos);
        }
    }

    // not handled
    return nullptr;
}

bool MeshMaterialListModel::handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& dragData)
{
    if (auto elem = data(item))
    {
        auto fileData = rtti_cast<ed::AssetBrowserFileDragDrop>(dragData);
        if (fileData && fileData->file())
        {
            const auto loadedMaterial = LoadResource<IMaterial>(fileData->file()->depotPath());
            const auto materialRef = MaterialRef(fileData->file()->depotPath().view(), loadedMaterial);
            return elem->baseMaterial(materialRef);
        }
    }

    return false;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshMaterialsPanel);
RTTI_END_TYPE();

MeshMaterialsPanel::MeshMaterialsPanel(ActionHistory* actionHistory)
    : m_captionsRefreshTimer(this, "UpdateCaptions"_id)
{
    layoutVertical();

    auto splitter = createChild<ui::Splitter>(ui::Direction::Horizontal, 0.4f);

    {
        actions().bindCommand("MaterialList.Highlight"_id) = [this]() {
            m_settings.highlight = !m_settings.highlight;
            call(EVENT_MATERIAL_SELECTION_CHANGED);
        };

        actions().bindCommand("MaterialList.Isolate"_id) = [this]() {
            m_settings.isolate = !m_settings.isolate;
            call(EVENT_MATERIAL_SELECTION_CHANGED);
        };

        actions().bindToggle("MaterialList.Highlight"_id) = [this]() { return m_settings.highlight; };
        actions().bindToggle("MaterialList.Isolate"_id) = [this]() { return m_settings.isolate; };

    }

    {
        auto panel = splitter->createChild<ui::IElement>();
        panel->expand();
        panel->layoutVertical();

        auto toolbar = panel->createChild<ui::ToolBar>();
        toolbar->createButton("MaterialList.Highlight"_id, ui::ToolbarButtonSetup().caption("Highlight"));
        toolbar->createButton("MaterialList.Isolate"_id, ui::ToolbarButtonSetup().caption("Isolate"));

        auto searchBar = panel->createChild<ui::SearchBar>(false);

        m_list = panel->createChild<ui::ListView>();
        m_list->expand();

        searchBar->bindItemView(m_list);

        m_listModel = RefNew<MeshMaterialListModel>();
        m_list->model(m_listModel);
    }

    {
        m_properties = splitter->createChild<ui::DataInspector>();
        m_properties->bindActionHistory(actionHistory);
        m_properties->expand();
    }

    {
        m_list->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            refreshMaterialProperties();
            call(EVENT_MATERIAL_SELECTION_CHANGED);
        };
    }

    m_captionsRefreshTimer = [this]() { updateCaptions(); };
    m_captionsRefreshTimer.startRepeated(0.05f);

    refreshMaterialList();
}

void MeshMaterialsPanel::bindResource(const MeshPtr& mesh)
{
    if (m_mesh != mesh)
    {
        m_mesh = mesh;
        refreshMaterialList();
        refreshMaterialProperties();
    }
}

void MeshMaterialsPanel::collectSelectedMaterialNames(HashSet<StringID>& outNames) const
{
    for (const auto& id : m_list->selection().keys())
        if (auto name = m_listModel->materialName(id))
            outNames.insert(name);
}

void MeshMaterialsPanel::showMaterials(const Array<StringID>& names)
{
    Array<ui::ModelIndex> selection;

    for (auto name : names)
        if (auto index = m_listModel->findMaterial(name))
            selection.pushBack(index);

    m_list->select(selection);

    if (!selection.empty())
        m_list->ensureVisible(selection[0]);
}

void MeshMaterialsPanel::refreshMaterialProperties()
{
    auto material = m_listModel->data(m_list->selectionRoot());
    m_properties->bindObject(material ? material->data() : nullptr);
}

void MeshMaterialsPanel::refreshMaterialList()
{
    StringID selectedMaterialName;

    if (m_list->selectionRoot())
    {
        auto material = m_listModel->data(m_list->selectionRoot());
        selectedMaterialName = material->name();
    }

    m_listModel->clear();

    RefPtr<MeshMaterialParameters> materialToSelect;
    if (m_mesh)
    {
        for (const auto& mat : m_mesh->materials())
        {
            auto entry = RefNew<MeshMaterialParameters>(mat.material, mat.name);
            m_listModel->add(entry);

            if (entry->name() == selectedMaterialName)
                materialToSelect = entry;
        }
    }

    if (auto index = m_listModel->index(materialToSelect))
        m_list->select(index);
}
    
void MeshMaterialsPanel::updateCaptions()
{
    for (uint32_t i=0; i<m_listModel->size(); ++i)
    {
        const auto& elem = m_listModel->at(i);
        if (elem->updateDisplayString())
        {
            if (auto index = m_listModel->index(elem))
                m_listModel->requestItemUpdate(index);
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(ed)
