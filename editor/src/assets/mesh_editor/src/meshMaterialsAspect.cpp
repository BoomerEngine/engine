/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#include "build.h"
#include "meshEditor.h"
#include "meshPreviewPanelWithToolbar.h"
#include "meshMaterialsAspect.h"

#include "assets/mesh_loader/include/rendernigMeshMaterialManifest.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

#include "base/editor/include/assetBrowser.h"
#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"

#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiSplitter.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiSearchBar.h"
#include "base/ui/include/uiDragDrop.h"

#include "base/resource/include/resourcePath.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/object/include/dataViewNative.h"
#include "base/object/include/rttiDataView.h"

namespace ed
{
    //---

    class MeshEditorMaterialInstance : public rendering::MaterialInstance
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshEditorMaterialInstance, rendering::MaterialInstance);

    public:
        MeshEditorMaterialInstance(rendering::MeshMaterialBindingManifest* manifest, base::StringID name, const rendering::MaterialInstancePtr& meshBaseParams)
            : m_manifest(manifest)
            , m_name(name)
            , m_originalMeshMaterialData(meshBaseParams)
        {            
            m_baseMaterial = meshBaseParams->baseMaterial();
            m_parameters = meshBaseParams->parameters();
            m_originalBaseMaterial = m_baseMaterial;

            m_manifest->applyMaterial(name, *this);

            createMaterialProxy();
        }

        virtual bool readParameterDefaultValue(base::StringView<char> viewPath, void* targetData, base::Type targetType) const override
        {
            return m_originalMeshMaterialData->readDataView(viewPath, targetData, targetType).valid();
        }

        virtual bool resetParameterOverride(const base::StringID name) override
        {
            if (name == "baseMaterial"_id)
            {
                baseMaterial(m_originalBaseMaterial);
                return true;
            }

            if (const auto materialTemplate = m_originalMeshMaterialData->resolveTemplate())
            {
                if (const auto* paramInfo = materialTemplate->findParameterInfo(name))
                {
                    base::rtti::DataHolder data(paramInfo->type);
                    if (m_originalMeshMaterialData->readParameterRaw(name, data.data(), data.type()))
                    {
                        return writeParameterRaw(name, data.data(), data.type());
                    }
                }
            }

            return TBaseClass::resetParameterOverride(name);
        }

        virtual bool hasParameterOverride(const base::StringID name) const override
        {
            if (name == "baseMaterial"_id)
                return baseMaterial() != m_originalBaseMaterial;

            for (const auto& param : parameters())
            {
                if (param.name == name && param.value)
                {
                    base::rtti::DataHolder holder(param.value.type());
                    if (!m_originalMeshMaterialData->readParameterRaw(name, holder.data(), holder.type()))
                        return true;
                    
                    return !holder.type()->compare(param.value.data(), holder.data());
                }
            }

            return false;
        }

        virtual void onPropertyChanged(base::StringView<char> path) override
        {
            TBaseClass::onPropertyChanged(path);
            m_manifest->captureMaterial(m_name, *this, m_originalMeshMaterialData);
        }

    private:
        rendering::MeshMaterialBindingManifest* m_manifest;
        base::StringID m_name;

        rendering::MaterialInstancePtr m_originalMeshMaterialData;
        rendering::MaterialRef m_originalBaseMaterial;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshEditorMaterialInstance);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshMaterialParameters);
    RTTI_END_TYPE();

    MeshMaterialParameters::MeshMaterialParameters(rendering::MeshMaterialBindingManifest* manifest, base::StringID name, const rendering::MaterialInstancePtr& meshBaseParams)
        : m_name(name)
        , m_manifest(manifest)
        , m_baseParams(meshBaseParams)
    {
        //m_previewParams = base::CreateSharedPtr<rendering::MaterialInstance>();
        //m_previewParams->baseMaterial(meshBaseParams);

        auto loader = base::GetService<base::res::LoadingService>()->loader();

        m_previewParams = base::CreateSharedPtr<MeshEditorMaterialInstance>(manifest, name, meshBaseParams);
        m_previewParams->parent(this);

        updateDisplayString();
    }

    bool MeshMaterialParameters::updateDisplayString()
    {
        base::StringBuilder txt;

        txt.append("[img:material] ");
        txt.append(m_name);

        if (m_previewParams)
        {
            if (auto materialTemplate = m_previewParams->resolveTemplate())
            {
                if (const auto fileName = materialTemplate->path().fileStem())
                {
                    txt.appendf(" [tag:#888]{}[/tag]", fileName);
                }

                bool hasTextureOverrides = false;
                bool hasParamOverrides = false;
                for (const auto& param : m_previewParams->parameters())
                {
                    if (m_previewParams->hasParameterOverride(param.name))
                    {
                        const auto metaType = param.value.type()->metaType();
                        if (metaType == base::rtti::MetaType::ResourceRef || metaType == base::rtti::MetaType::AsyncResourceRef)
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

    bool MeshMaterialParameters::baseMaterial(const rendering::MaterialRef& material)
    {
        m_previewParams->baseMaterial(material);
        return true;
    }

    //---

    MeshMaterialListModel::MeshMaterialListModel(rendering::MeshMaterialBindingManifest* manifest)
        : m_manifest(manifest)
    {}

    bool MeshMaterialListModel::compare(const base::RefPtr<MeshMaterialParameters>& a, const base::RefPtr<MeshMaterialParameters>& b, int colIndex) const
    {
        return a->name().view() < b->name().view();
    }

    bool MeshMaterialListModel::filter(const base::RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        return filter.testString(data->name().view());
    }

    base::StringBuf MeshMaterialListModel::content(const base::RefPtr<MeshMaterialParameters>& data, int colIndex /*= 0*/) const
    {
        return data->displayString();
    }

    ui::DragDropHandlerPtr MeshMaterialListModel::handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& dragData, const ui::Position& pos)
    {
        if (auto elem = data(item))
        {
            // can we handle this data ?
            auto fileData = base::rtti_cast<ed::AssetBrowserFileDragDrop>(dragData);
            if (fileData && fileData->file())
            {
                if (fileData->file()->fileFormat().loadableAsType(rendering::IMaterial::GetStaticClass()))
                    return base::CreateSharedPtr<ui::DragDropHandlerGeneric>(dragData, view, pos);
            }
        }

        // not handled
        return nullptr;
    }

    bool MeshMaterialListModel::handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& dragData)
    {
        if (auto elem = data(item))
        {
            auto fileData = base::rtti_cast<ed::AssetBrowserFileDragDrop>(dragData);
            if (fileData && fileData->file())
            {
                if (auto material = base::LoadResource<rendering::IMaterial>(base::res::ResourcePath(fileData->file()->depotPath())))
                {
                    return elem->baseMaterial(material);
                }
            }
        }

        return false;
    }

    //---

    RTTI_BEGIN_TYPE_CLASS(MeshMaterialBindingAspect);
    RTTI_END_TYPE();

    MeshMaterialBindingAspect::MeshMaterialBindingAspect()
        : SingleResourceEditorManifestAspect(rendering::MeshMaterialBindingManifest::GetStaticClass())
    {
    }

    bool MeshMaterialBindingAspect::initialize(SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        m_materialManifest = base::rtti_cast<rendering::MeshMaterialBindingManifest>(manifest());
        if (!m_materialManifest)
            return false;

        if (auto* meshEditor = base::rtti_cast<MeshEditor>(editor))
        {
            m_meshEditor = meshEditor;
            m_previewPanel = meshEditor->previewPanel();

            m_timer.create(meshEditor, "UpdateCaptions"_id);
            m_timer->startRepeated(0.05f);
            *m_timer = [this]() { updateCaptions(); };

            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:color] Materials");
            panel->layoutVertical();

            auto splitter = panel->createChild<ui::Splitter>(ui::Direction::Horizontal, 0.4f);

            {
                auto panel = splitter->createChild<ui::IElement>();
                panel->expand();
                panel->layoutVertical();

                auto searchBar = panel->createChild<ui::SearchBar>(false);

                m_list = panel->createChild<ui::ListView>();
                m_list->expand();

                searchBar->bindItemView(m_list);

                m_listModel = base::CreateSharedPtr<MeshMaterialListModel>(m_materialManifest);
                m_list->model(m_listModel);

            }

            {
                m_properties = splitter->createChild<ui::DataInspector>();
                m_properties->bindActionHistory(editor->actionHistory());
                m_properties->expand();
            }

            {
                m_list->OnSelectionChanged = [this]()
                {
                    refreshMaterialProperties();
                };
            }

            refreshMaterialList();

            meshEditor->dockLayout().right().attachPanel(panel);
            return true;
        }

        return false;
    }

    void MeshMaterialBindingAspect::previewResourceChanged()
    {
        refreshMaterialList();
        refreshMaterialProperties();
    }

    void MeshMaterialBindingAspect::refreshMaterialProperties()
    {
        auto material = m_listModel->data(m_list->selectionRoot());
        m_properties->bindObject(material ? material->previewParams() : nullptr);
    }

    void MeshMaterialBindingAspect::refreshMaterialList()
    {
        base::StringID selectedMaterialName;

        if (m_list->selectionRoot())
        {
            auto material = m_listModel->data(m_list->selectionRoot());
            selectedMaterialName = material->name();
        }

        m_listModel->clear();

        base::RefPtr<MeshMaterialParameters> materialToSelect;
        if (auto mesh = m_meshEditor->previewMesh())
        {
            for (const auto& mat : mesh->materials())
            {
                auto entry = base::CreateSharedPtr<MeshMaterialParameters>(m_materialManifest, mat.name, mat.baseMaterial);
                m_listModel->add(entry);

                m_previewPanel->previewMaterial(mat.name, entry->previewParams());

                if (entry->name() == selectedMaterialName)
                    materialToSelect = entry;
            }
        }

        if (auto index = m_listModel->index(materialToSelect))
            m_list->select(index);
    }
    
    void MeshMaterialBindingAspect::updateCaptions()
    {
        for (auto elem : m_listModel->elements())
        {
            if (elem->updateDisplayString())
            {
                if (auto index = m_listModel->index(elem))
                    m_listModel->requestItemUpdate(index);
            }
        }
    }

    //---

} // ed