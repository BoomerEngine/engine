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

#include "editor/assets/include/browserService.h"
#include "editor/common/include/utils.h"

#include "engine/mesh/include/mesh.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"

#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiListViewEx.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiDragDrop.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiCheckBox.h"
#include "engine/ui/include/uiToolBar.h"

#include "core/resource/include/loader.h"
#include "core/resource/include/depot.h"
#include "core/object/include/dataViewNative.h"
#include "core/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshMaterialParameters);
RTTI_END_TYPE();

MeshMaterialParameters::MeshMaterialParameters(MaterialInstance* data, StringID name)
    : m_name(name)
    , m_data(AddRef(data))
    , m_events(this)
{
    layoutHorizontal();

    {
        m_label = createChild<ui::TextLabel>();
        m_label->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    }

    m_events.bind(m_data->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]()
    {
        updateDisplayString();
    };

    updateDisplayString();
}

void MeshMaterialParameters::updateDisplayString()
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

    m_label->text(txt.view());
}

void MeshMaterialParameters::baseMaterial(const MaterialRef& material)
{
    if (m_data)
    {
        m_data->baseMaterial(material);
        updateDisplayString();
    }
}

bool MeshMaterialParameters::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    return filter.testString(m_name.view());
}

void MeshMaterialParameters::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const
{
    outInfo.index = uniqueIndex();
    outInfo.caption = m_name.view();
}

bool MeshMaterialParameters::handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, InputKeyMask controlKeys)
{
    return false;
}

bool MeshMaterialParameters::handleItemActivate(ui::ICollectionView* view)
{
    if (m_data)
    {

    }

    return true;
}

ui::DragDropHandlerPtr MeshMaterialParameters::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
        if (CanLoadAsClass<IMaterial>(fileData->depotPath()))
            return RefNew<ui::DragDropHandlerGeneric>(fileData, this, entryPosition);

    return nullptr;
}

void MeshMaterialParameters::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
        if (auto ref = LoadResourceRef<IMaterial>(fileData->depotPath()))
            baseMaterial(ref);
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshMaterialsPanel);
RTTI_END_TYPE();

MeshMaterialsPanel::MeshMaterialsPanel(ActionHistory* actionHistory)
{
    layoutVertical();

    auto splitter = createChild<ui::Splitter>(ui::Direction::Horizontal, 0.4f);

    {
        auto panel = splitter->createChild<ui::IElement>();
        panel->expand();
        panel->layoutVertical();

        m_toolbar = panel->createChild<ui::ToolBar>();
        m_toolbar->createButton(ui::ToolBarButtonInfo("Highlight"_id).caption("Highlight", "highlight")) = [this]()
        {
            m_settings.highlight = !m_settings.highlight;
            updateToolbar();
            call(EVENT_MATERIAL_SELECTION_CHANGED);
        };

        m_toolbar->createButton(ui::ToolBarButtonInfo("Isolate"_id).caption("Isolate", "html")) = [this]()
        {
            m_settings.isolate = !m_settings.isolate;
            updateToolbar();
            call(EVENT_MATERIAL_SELECTION_CHANGED);
        };

        auto searchBar = panel->createChild<ui::SearchBar>(false);

        m_list = panel->createChild<ui::ListViewEx>();
        m_list->expand();

        searchBar->bindItemView(m_list);
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

    refreshMaterialList();
    updateToolbar();
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
    m_list->selection().visit<MeshMaterialParameters>([&outNames](MeshMaterialParameters* param)
        {
            outNames.insert(param->name());
        });
}

void MeshMaterialsPanel::showMaterials(const Array<StringID>& names)
{
    ui::CollectionItems items;
    m_list->iterate<MeshMaterialParameters>([&items, &names](MeshMaterialParameters* param)
        {
            if (names.contains(param->name()))
                items.add(param);
        });

    m_list->select(items);

    if (items.first<MeshMaterialParameters>())
        m_list->ensureVisible(items.first<MeshMaterialParameters>());
}

void MeshMaterialsPanel::refreshMaterialProperties()
{
    Array<MaterialInstancePtr> objects;

    m_list->selection().visit<MeshMaterialParameters>([&objects](MeshMaterialParameters* param)
        {
            if (auto data = param->data())
                objects.pushBack(data);
        });

    m_properties->bindObjects(objects);
}

StringID MeshMaterialsPanel::selectedMaterial() const
{
    if (auto mat = m_list->selection().first<MeshMaterialParameters>())
        return mat->name();
    return StringID();
}

void MeshMaterialsPanel::refreshMaterialList()
{
    auto selected = selectedMaterial();

    m_list->clear();

    RefPtr<MeshMaterialParameters> materialToSelect;
    if (m_mesh)
    {
        for (const auto& mat : m_mesh->materials())
        {
            auto entry = RefNew<MeshMaterialParameters>(mat.material, mat.name);
            m_list->addItem(entry);

            if (entry->name() == selected)
                m_list->select(entry);
        }
    }
}
    
void MeshMaterialsPanel::updateToolbar()
{
    m_toolbar->toggleButton("Highlight"_id, m_settings.highlight);
    m_toolbar->toggleButton("Isolate"_id, m_settings.isolate);
}

//---

END_BOOMER_NAMESPACE_EX(ed)
