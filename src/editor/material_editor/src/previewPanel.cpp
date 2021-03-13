/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "previewPanel.h"

#include "editor/common/include/utils.h"
#include "editor/assets/include/browserService.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/mesh/include/mesh.h"
#include "engine/material/include/material.h"

#include "engine/rendering/include/scene.h"
#include "engine/rendering/include/object.h"
#include "engine/rendering/include/objectMesh.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

static StaticResource<Mesh> resDefaultCustomMesh("/engine/meshes/teapot.xfile");

MaterialPreviewPanelSettings::MaterialPreviewPanelSettings()
{
    customMesh = resDefaultCustomMesh.load().resource();
}

//--
     
RTTI_BEGIN_TYPE_CLASS(MaterialPreviewPanel);
RTTI_PROPERTY(m_material);
RTTI_END_TYPE();

MaterialPreviewPanel::MaterialPreviewPanel()
{
    //m_panelSettings.cameraForceOrbit = true;
    m_panelSettings.drawInternalGrid = true;
    m_panelSettings.drawInternalWorldAxis = false;
    m_panelSettings.drawInternalCameraAxis = false;
    m_panelSettings.drawInternalCameraData = false;

    auto settings = cameraSettings();
    settings.mode = ui::CameraMode::OrbitPerspective;
    settings.rotation = Angles(35.0f, 50.0f, 0.0f);
    settings.position = settings.rotation.forward() * -2.0f;
    cameraSettings(settings);

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

void MaterialPreviewPanel::bindMaterial(const IMaterial* material)
{
    destroyVisualization();
    m_material = AddRef(material);
    createVisualization();
}

bool MaterialPreviewPanel::computeContentBounds(Box& outBox) const
{
    outBox = Box(Vector3::ZERO(), 0.5f);
    return true;
}

void MaterialPreviewPanel::destroyVisualization()
{
    if (m_previewProxy)
    {
        scene()->manager<rendering::ObjectManagerMesh>()->detachProxy(m_previewProxy);
        m_previewProxy.reset();
    }
}

ui::DragDropHandlerPtr MaterialPreviewPanel::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
        if (auto file = fileData->depotPath())
            if (CanLoadAsClass<Mesh>(file))
                return RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);

    return TBaseClass::handleDragDrop(data, entryPosition);
}

void MaterialPreviewPanel::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
    {
        if (auto file = fileData->depotPath())
        {
            if (CanLoadAsClass<Mesh>(file))
            {
                auto settings = previewSettings();

                // TODO: async
                if (auto loadedMash = LoadResource<Mesh>(file))
                {
                    settings.customMesh = loadedMash;
                    settings.shape = MaterialPreviewShape::Custom;
                    previewSettings(settings);
                }
            }
        }
    }
}

StaticResource<Mesh> resBoxMesh("/engine/meshes/cube.xfile");
StaticResource<Mesh> resBoxSphere("/engine/meshes/sphere.xfile");
StaticResource<Mesh> resBoxCylinder("/engine/meshes/cylinder.xfile");
//StaticResource<Mesh> resBoxQuad("/engine/meshes/quad.xfile");
StaticResource<Mesh> resBoxQuad("/engine/meshes/plane.xfile");

void MaterialPreviewPanel::createVisualization()
{
    destroyVisualization();

    if (auto material = m_material)
    {
        auto previewTemplate = material->resolveTemplate();

        if (previewTemplate)
        {
            MeshPtr mesh;
            switch (m_previewSettings.shape)
            {
                case MaterialPreviewShape::Box: mesh = resBoxMesh.load().resource(); break;
                case MaterialPreviewShape::Sphere: mesh = resBoxSphere.load().resource(); break;
                case MaterialPreviewShape::Cylinder: mesh = resBoxCylinder.load().resource(); break;
                case MaterialPreviewShape::Plane: mesh = resBoxQuad.load().resource(); break;
                case MaterialPreviewShape::Custom: mesh = m_previewSettings.customMesh; break;
            }

            if (mesh)
            {
                rendering::ObjectProxyMesh::Setup desc;
                desc.mesh = mesh;
                desc.forcedLodLevel = 0;
                desc.forceMaterial = m_material;

                if (auto proxy = rendering::ObjectProxyMesh::Compile(desc))
                {
                    const auto minZ = mesh->bounds().min.z;
                    const auto offset = Vector3(0, 0, -minZ);
                    proxy->m_localToWorld.translation(offset);

                    m_previewProxy = proxy;
                    scene()->manager<rendering::ObjectManagerMesh>()->attachProxy(m_previewProxy);

                    focusOnBounds(mesh->bounds() + offset, 1.0f, nullptr);
                }
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
    toolbar()->createButton(ui::ToolBarButtonInfo("Shape"_id).caption("[img:cube] Shape")) = 
        [this](ui::Button* button) {
            auto menu = RefNew<ui::MenuButtonContainer>();
            buildShapePopup(menu);
            menu->showAsDropdown(button);
        };
    toolbar()->createSeparator();
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
