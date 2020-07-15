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
#include "base/editor/include/assetBrowser.h"
#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/ui/include/uiToolBar.h"

namespace ed
{

    //--

    static base::res::StaticResource<rendering::Mesh> resDefaultCustomMesh("engine/tests/meshes/teapot.obj");

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
        setupCameraForceOrbitMode(true);
        setupCameraAtAngle(base::Angles(35.0f, 50.0f, 0.0f), 2.0f);

        m_drawInternalGrid = true;
        m_drawInternalWorldAxis = false;
        m_drawInternalCameraAxis = false;
        m_drawInternalCameraData = false;
    }

    MaterialPreviewPanel::~MaterialPreviewPanel()
    {
        destroyVisualization();
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

    void MaterialPreviewPanel::onPropertyChanged(base::StringView<char> path)
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
                    return base::CreateSharedPtr<ui::DragDropHandlerGeneric>(data, this, entryPosition);

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
                    if (auto loadedMash = base::LoadResource<rendering::Mesh>(base::res::ResourcePath(file->depotPath())))
                    {
                        settings.customMesh = loadedMash;
                        settings.shape = MaterialPreviewShape::Custom;
                        previewSettings(settings);
                    }
                }
            }
        }
    }

    base::res::StaticResource<rendering::Mesh> resBoxMesh("engine/tests/meshes/cube.obj");
    base::res::StaticResource<rendering::Mesh> resBoxSphere("engine/tests/meshes/sphere.obj");
    base::res::StaticResource<rendering::Mesh> resBoxCylinder("engine/tests/meshes/cylinder.obj");
    base::res::StaticResource<rendering::Mesh> resBoxQuad("engine/tests/meshes/quad.obj");

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

    RTTI_BEGIN_TYPE_CLASS(MaterialPreviewPanelWithToolbar);
    RTTI_END_TYPE();

    MaterialPreviewPanelWithToolbar::MaterialPreviewPanelWithToolbar()
    {
        layoutVertical();

        actions().bindCommand("MaterialPreviewPanel.ShapeBox"_id) = [this]() { changePreviewShape(MaterialPreviewShape::Box); };
        actions().bindCommand("MaterialPreviewPanel.ShapeCylinder"_id) = [this]() { changePreviewShape(MaterialPreviewShape::Cylinder); };
        actions().bindCommand("MaterialPreviewPanel.ShapeSphere"_id) = [this]() { changePreviewShape(MaterialPreviewShape::Sphere); };
        actions().bindCommand("MaterialPreviewPanel.ShapePlane"_id) = [this]() { changePreviewShape(MaterialPreviewShape::Plane); };
        actions().bindCommand("MaterialPreviewPanel.ShapeCustom"_id) = [this]() { changePreviewShape(MaterialPreviewShape::Custom); };

        actions().bindToggle("MaterialPreviewPanel.ShapeBox"_id) = [this]() { return previewSettings().shape == MaterialPreviewShape::Box; };
        actions().bindToggle("MaterialPreviewPanel.ShapeCylinder"_id) = [this]() { return previewSettings().shape == MaterialPreviewShape::Cylinder; };
        actions().bindToggle("MaterialPreviewPanel.ShapeSphere"_id) = [this]() { return previewSettings().shape == MaterialPreviewShape::Sphere; };
        actions().bindToggle("MaterialPreviewPanel.ShapePlane"_id) = [this]() { return previewSettings().shape == MaterialPreviewShape::Plane; };
        actions().bindToggle("MaterialPreviewPanel.ShapeCustom"_id) = [this]() { return previewSettings().shape == MaterialPreviewShape::Custom; };

        m_toolbar = createChild<ui::ToolBar>();
        m_toolbar->createButton("MaterialPreviewPanel.ShapeBox"_id, ui::ToolbarButtonSetup().caption("Cube"));
        m_toolbar->createButton("MaterialPreviewPanel.ShapeCylinder"_id, ui::ToolbarButtonSetup().caption("Cylinder"));
        m_toolbar->createButton("MaterialPreviewPanel.ShapeSphere"_id, ui::ToolbarButtonSetup().caption("Sphere"));
        m_toolbar->createButton("MaterialPreviewPanel.ShapePlane"_id, ui::ToolbarButtonSetup().caption("Quad"));
        m_toolbar->createButton("MaterialPreviewPanel.ShapeCustom"_id, ui::ToolbarButtonSetup().caption("Custom"));

        m_panel = createChild<MaterialPreviewPanel>();
        m_panel->expand();
    }

    MaterialPreviewPanelWithToolbar::~MaterialPreviewPanelWithToolbar()
    {}

    void MaterialPreviewPanelWithToolbar::changePreviewShape(MaterialPreviewShape shape)
    {
        auto settings = previewSettings();
        if (settings.shape != shape)
        {
            settings.shape = shape;
            previewSettings(settings);
        }
    }

    const MaterialPreviewPanelSettings& MaterialPreviewPanelWithToolbar::previewSettings() const
    {
        return m_panel->previewSettings();
    }

    void MaterialPreviewPanelWithToolbar::previewSettings(const MaterialPreviewPanelSettings& settings)
    {
        m_panel->previewSettings(settings);
    }

    void MaterialPreviewPanelWithToolbar::bindMaterial(const rendering::MaterialRef& material)
    {
        m_panel->bindMaterial(material);
    }

    //--
    
} // ed
