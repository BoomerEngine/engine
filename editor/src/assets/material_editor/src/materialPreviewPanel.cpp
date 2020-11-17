/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"

#include "materialPreviewPanel.h"
#include "rendering/scene/include/renderingSceneProxy.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/material/include/renderingMaterial.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingScene.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/editor/include/assetBrowser.h"
#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/ui/include/uiToolBar.h"

namespace ed
{

    //--

    static base::res::StaticResource<rendering::Mesh> resDefaultCustomMesh("/engine/meshes/teapot.v4mesh");

    MaterialPreviewPanelSettings::MaterialPreviewPanelSettings()
    {
        customMesh = resDefaultCustomMesh.loadAndGetAsRef();
    }

    //--
     
    RTTI_BEGIN_TYPE_CLASS(MaterialPreviewPanel);
    RTTI_PROPERTY(m_material);
    RTTI_END_TYPE();

    MaterialPreviewPanel::MaterialPreviewPanel()
    {
        m_panelSettings.cameraForceOrbit = true;
        m_panelSettings.drawInternalGrid = true;
        m_panelSettings.drawInternalWorldAxis = false;
        m_panelSettings.drawInternalCameraAxis = false;
        m_panelSettings.drawInternalCameraData = false;

        const auto rotation = base::Angles(35.0f, 50.0f, 0.0f);
        setupCamera(rotation, rotation.forward() * -2.0f);

        createToolbarItems();
    }

    MaterialPreviewPanel::~MaterialPreviewPanel()
    {
        destroyVisualization();
    }

    void MaterialPreviewPanel::previewShape(MaterialPreviewShape shape)
    {
        auto settings = m_previewSettings;
        settings.shape = shape;
        previewSettings(settings);
    }

    void MaterialPreviewPanel::previewSettings(const MaterialPreviewPanelSettings& settings)
    {
        auto oldShape = m_previewSettings.shape;
        auto oldMesh = m_previewSettings.customMesh;

        m_previewSettings = settings;

        if (m_previewSettings.shape != oldShape || m_previewSettings.customMesh != oldMesh)
        {
            destroyVisualization();
            createVisualization();
        }
    }

    void MaterialPreviewPanel::onPropertyChanged(base::StringView path)
    {
        TBaseClass::onPropertyChanged(path);

        if (path == "material")
        {
            destroyVisualization();
            createVisualization();
        }
    }

    void MaterialPreviewPanel::bindMaterial(const rendering::MaterialRef& material)
    {
        if (m_material != material)
        {
            m_material = material;
            onPropertyChanged("material");
        }
    }

    bool MaterialPreviewPanel::computeContentBounds(base::Box& outBox) const
    {
        outBox = base::Box(base::Vector3::ZERO(), 0.5f);
        return true;
    }

    void MaterialPreviewPanel::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);
    }

    void MaterialPreviewPanel::destroyVisualization()
    {
        if (m_previewProxy)
        {
            renderingScene()->proxyDestroy(m_previewProxy);
            m_previewProxy.reset();
        }
    }

    ui::DragDropHandlerPtr MaterialPreviewPanel::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
    {
        if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
            if (auto file = fileData->file())
                if (file->fileFormat().loadableAsType(rendering::Mesh::GetStaticClass()))
                    return base::RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);

        return TBaseClass::handleDragDrop(data, entryPosition);
    }

    void MaterialPreviewPanel::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
    {
        if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
        {
            if (auto file = fileData->file())
            {
                if (file->fileFormat().loadableAsType(rendering::Mesh::GetStaticClass()))
                {
                    auto settings = previewSettings();

                    // TODO: async
                    if (auto loadedMash = base::LoadResource<rendering::Mesh>(file->depotPath()))
                    {
                        settings.customMesh = loadedMash;
                        settings.shape = MaterialPreviewShape::Custom;
                        previewSettings(settings);
                    }
                }
            }
        }
    }

    base::res::StaticResource<rendering::Mesh> resBoxMesh("/engine/meshes/cube.v4mesh");
    base::res::StaticResource<rendering::Mesh> resBoxSphere("/engine/meshes/sphere.v4mesh");
    base::res::StaticResource<rendering::Mesh> resBoxCylinder("/engine/meshes/cylinder.v4mesh");
    base::res::StaticResource<rendering::Mesh> resBoxQuad("/engine/meshes/quad.v4mesh");

    void MaterialPreviewPanel::createVisualization()
    {
        destroyVisualization();

        if (auto material = m_material.acquire())
        {
            auto previewTemplate = material->resolveTemplate();

            if (previewTemplate)
            {
                rendering::MeshPtr mesh;
                switch (m_previewSettings.shape)
                {
                case MaterialPreviewShape::Box: mesh = resBoxMesh.loadAndGet(); break;
                case MaterialPreviewShape::Sphere: mesh = resBoxSphere.loadAndGet(); break;
                case MaterialPreviewShape::Cylinder: mesh = resBoxCylinder.loadAndGet(); break;
                case MaterialPreviewShape::Plane: mesh = resBoxQuad.loadAndGet(); break;
                case MaterialPreviewShape::Custom: mesh = m_previewSettings.customMesh.acquire(); break;
                }

                if (mesh)
                {
                    rendering::scene::ProxyMeshDesc desc;
                    desc.mesh = mesh;
                    desc.autoHideDistanceOverride = 10000.0f;
                    desc.forcedLodLevel = 0;
                    desc.forceMaterial = m_material.acquire()->dataProxy();
                    desc.meshBounds = mesh->bounds();

                    const auto minZ = mesh->bounds().min.z;
                    const auto offset = base::Vector3(0, 0, -minZ);
                    desc.localToScene.translation(offset);

                    // register proxy
                    m_previewProxy = renderingScene()->proxyCreate(desc);

                    // zoom out
                    setupCameraAroundBounds(mesh->bounds() + offset);
                }
            }
        }
    }

    //--

    void MaterialPreviewPanel::buildShapePopup(ui::MenuButtonContainer* menu)
    {
        // default modes
        menu->createCallback("Cube", "") = [this]() { previewShape(MaterialPreviewShape::Box); };
        menu->createCallback("Cylinder", "") = [this]() { previewShape(MaterialPreviewShape::Cylinder); };
        menu->createCallback("Sphere", "") = [this]() { previewShape(MaterialPreviewShape::Sphere); };
        menu->createCallback("Plane", "") = [this]() { previewShape(MaterialPreviewShape::Plane); };
        menu->createSeparator();
        menu->createCallback("Custom", "") = [this]() { previewShape(MaterialPreviewShape::Custom); };
    }

    void MaterialPreviewPanel::createToolbarItems()
    {
        actions().bindCommand("MaterialPreviewPanel.Shape"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<ui::MenuButtonContainer>();
                buildShapePopup(menu);
                menu->showAsDropdown(button);
            }
        };

        toolbar()->createButton("MaterialPreviewPanel.Shape"_id, ui::ToolbarButtonSetup().caption("[img:cube] Shape"));
        toolbar()->createSeparator();
    }

    //--
    
} // ed
