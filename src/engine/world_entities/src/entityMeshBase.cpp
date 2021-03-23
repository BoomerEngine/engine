/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "entityMeshBase.h"

#include "engine/mesh/include/mesh.h"
#include "engine/rendering/include/objectMesh.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(MeshEntity);
    RTTI_CATEGORY("Mesh");
    RTTI_PROPERTY(m_castShadows).overriddable().editable("Should mesh cast shadows");
    RTTI_PROPERTY(m_receiveShadows).overriddable().editable("Should mesh receive shadows");
    RTTI_PROPERTY(m_forceDetailLevel).overriddable().editable("Force single LOD of the mesh");
    RTTI_PROPERTY(m_forceDistance).overriddable().editable("Force visibility distance for mesh");
    RTTI_CATEGORY("Effects");
    RTTI_PROPERTY(m_color).overriddable().editable("General effect color");
    RTTI_PROPERTY(m_colorEx).overriddable().editable("Secondary effect color");

    RTTI_PROPERTY(m_mesh);
RTTI_END_TYPE();

MeshEntity::MeshEntity()
{}

MeshEntity::~MeshEntity()
{}

void MeshEntity::castingShadows(bool flag)
{
    if (m_castShadows != flag)
    {
        m_castShadows = flag;
        // TODO: refresh proxy via command
    }
}

void MeshEntity::receivingShadows(bool flag)
{
    if (m_receiveShadows != flag)
    {
        m_receiveShadows = flag;
        // TODO: refresh proxy via command
    }
}

void MeshEntity::forceDetailLevel(char level)
{
    if (m_forceDetailLevel != level)
    {
        m_forceDetailLevel = level;
        recreateRenderingProxy(); // TODO: maybe that's to harsh ?
    }
}

void MeshEntity::forceDistance(float distance)
{
    const auto dist = (uint16_t)std::clamp<float>(distance, 0, 32767.0f); // leave higher bits for something in the future
    if (dist != m_forceDistance)
    {
        m_forceDistance = dist;
        recreateRenderingProxy(); // TODO: maybe that's to harsh ?
    }
}

void MeshEntity::color(Color color)
{
    if (m_color != color)
    {
        m_color = color;
        recreateRenderingProxy(); // TODO: maybe that's to harsh ?
    }
}

void MeshEntity::colorEx(Color color)
{
    if (m_colorEx != color)
    {
        m_colorEx = color;
        recreateRenderingProxy(); // TODO: maybe that's to harsh ?
    }
}

void MeshEntity::createRenderingProxy(rendering::RenderingScene* scene, rendering::RenderingObjectPtr& outProxy, rendering::IRenderingObjectManager*& outManager) const
{
    if (const auto meshData = m_mesh.resource())
    {
        if (auto manager = scene->manager<rendering::RenderingMeshManager>())
        {
            rendering::RenderingMesh::Setup setup;
            setup.forcedLodLevel = m_forceDetailLevel;
            setup.mesh = meshData;

            if (auto proxy = rendering::RenderingMesh::Compile(setup))
            {
                if (m_castShadows)
                    proxy->m_flags |= rendering::RenderingObjectFlagBit::CastShadows;
                if (m_receiveShadows)
                    proxy->m_flags |= rendering::RenderingObjectFlagBit::ReceivesShadows;

                const auto selected = editorState().selected;
                if (selected)
                    proxy->m_flags |= rendering::RenderingObjectFlagBit::Selected;

                proxy->m_color = m_color;
                proxy->m_colorEx = m_colorEx;
                proxy->m_localToWorld = cachedLocalToWorldMatrix();
                proxy->m_selectable = editorState().selectable;

                outProxy = proxy;
                outManager = manager;
            }
        }
    }
}

Box MeshEntity::calcBounds() const
{
    if (auto mesh = m_mesh.resource())
    {
        if (!mesh->bounds().empty())
        {
            return BaseTransformation(cachedLocalToWorldMatrix()).transformBox(mesh->bounds());
        }
    }

    return TBaseClass::calcBounds();
}

void MeshEntity::mesh(const MeshRef& mesh)
{
    if (m_mesh != mesh)
    {
        m_mesh = mesh;
        recreateRenderingProxy();
    }
}

//--

void MeshEntity::queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const
{
    TBaseClass::queryTemplateProperties(outTemplateProperties);
    outTemplateProperties.prop<MeshAsyncRef>("Mesh"_id, "mesh"_id, MeshAsyncRef(), PropertyEditorData().comment("Mesh to render").primaryResource());
}

bool MeshEntity::initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties)
{
    if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
        return false;

    m_mesh = templateProperties.compileValueOrDefault<MeshRef>("mesh"_id);
    if (!m_mesh)
    {
        static const auto defaultMesh = LoadResourceRef<Mesh>("/engine/meshes/box.v4mesh");
        m_mesh = defaultMesh;
    }

    return true;
}

//--

END_BOOMER_NAMESPACE()
