/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"
#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneEditMode_Default_Clipboard.h"

#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"
#include "scenePreviewPanel.h"
#include "sceneObjectPalettePanel.h"

#include "editor/assets/include/browserService.h"
#include "editor/common/include/utils.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiClassPickerBox.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiDragDrop.h"

#include "engine/world/include/world.h"
#include "engine/world/include/worldEntity.h"
#include "engine/world/include/rawEntity.h"
#include "engine/world/include/rawPrefab.h"

#include "engine/rendering/include/debug.h"

#include "core/object/include/actionHistory.h"
#include "core/object/include/action.h"
#include "core/resource/include/indirectTemplate.h"
#include "core/resource/include/indirectTemplateCompiler.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

extern void BindResourceToObjectTemplate(ObjectIndirectTemplate* ptr, StringView file);

//--

class SceneObjectDragDropPreview : public IReferencable
{
public:
    SceneObjectDragDropPreview()
    {}

    virtual ~SceneObjectDragDropPreview()
    {
    }

    INLINE bool ready() const { return m_ready.load(); }

    void buildEntityPreview(ClassType entityClass, StringView file)
    {
        auto compTemplate = RefNew<ObjectIndirectTemplate>();
        compTemplate->templateClass(entityClass);
        BindResourceToObjectTemplate(compTemplate, file);

        ObjectIndirectTemplateCompiler compiler;
        compiler.addTemplate(compTemplate);

        if (auto finalEntityClass = compiler.compileClass().cast<Entity>())
        {
            auto defaultEntity = (Entity*)finalEntityClass->defaultObject();
            finalEntityClass = defaultEntity->determineEntityTemplateClass(compiler);
            if (finalEntityClass)
            {
                if (auto entity = finalEntityClass->create<Entity>())
                {
                    if (entity->initializeFromTemplateProperties(compiler))
                    {
                        auto& info = m_entities.emplaceBack();
                        info.entity = entity;
                    }
                }
            }
        }

        m_ready.exchange(true);
    }

    void buildPrefabPreview(StringView file)
    {
        if (const auto prefab = LoadResource<Prefab>(file))
        {
            InplaceArray<EntityPtr, 20> allEntities;
            prefab->compile(StringID(), allEntities, true);

            for (const auto& ent : allEntities)
            {
                auto& info = m_entities.emplaceBack();
                info.entity = ent;
                info.relativePlacementTransform = ent->transform();
            }
        }

        m_ready.exchange(true);
    }


    void move(const Transform& placement)
    {
        m_placement = placement;

        for (const auto& ent : m_entities)
        {
            auto entityPlacement = placement * ent.relativePlacementTransform;
            ent.entity->requestTransformChangeWorldSpace(entityPlacement);
        }
    }

    void attach(World* world)
    {
        for (const auto& ent : m_entities)
            world->attachEntity(ent.entity);
    }

    void detach(World* world)
    {
        for (const auto& ent : m_entities)
            world->detachEntity(ent.entity);
    }
        
private:
    Transform m_placement;

    struct EntityInfo
    {
        EntityPtr entity;
        Transform relativePlacementTransform;
    };

    Array<EntityInfo> m_entities;

    std::atomic<bool> m_ready = false;
};

//--

class SceneObjectDragDropCreationHandler : public ui::IDragDropHandler
{
public:
    typedef std::function<void(const Transform&)> TCreateFunc;

    SceneObjectDragDropCreationHandler(const ui::DragDropDataPtr& data, ui::IElement* target, const ui::Position& initialPosition, ui::RenderingPanelDepthBufferQuery* depthData, World* world, const SceneGridSettings& gridSettings, const TCreateFunc& createFunc, SceneObjectDragDropPreview* preview)
        : ui::IDragDropHandler(data, target, initialPosition)
        , m_gridSettings(gridSettings)
        , m_depthBuffer(AddRef(depthData))
        , m_createFunc(createFunc)
        , m_loadingPreview(AddRef(preview))
        , m_world(world)
    {
    }

    virtual ~SceneObjectDragDropCreationHandler()
    {
        destroyPreview();
    }

    virtual void handleCancel() override
    {
        destroyPreview();
    }

    virtual void handleData(const ui::Position& absolutePos) const override
    {            
        updatePosition(absolutePos);

        if (m_positionUnderCursorValid)
        {
            m_createFunc(m_positionUnderCursor);
        }

        target()->focus();
    }

    virtual bool canHandleDataAt(const ui::Position& absolutePos) const override
    {
        updatePosition(absolutePos);
        return m_positionUnderCursorValid;
    }

    virtual bool canHideDefaultDataPreview() const override
    {
        return true;
    }

    void updatePosition(const ui::Position& absolutePos) const
    {
        Point clientPos = absolutePos - target()->cachedDrawArea().absolutePosition();

        ExactPosition worldPos;
        if (m_depthBuffer->calcWorldPosition(clientPos.x, clientPos.y, worldPos))
        {
            if (m_gridSettings.positionGridEnabled)
                m_positionUnderCursor = worldPos.snapped(m_gridSettings.positionGridSize);
            else
                m_positionUnderCursor = worldPos;

            m_positionUnderCursorValid = true;
        }
        else
        {
            m_positionUnderCursorValid = false;
        }
    }

    //--

    void destroyPreview()
    {
        if (m_currentPreview)
        {
            m_currentPreview->detach(m_world);
            m_currentPreview.reset();
        }

        m_loadingPreview.reset();
    }

    void update()
    {
        if (m_loadingPreview && m_loadingPreview->ready())
        {
            if (m_currentPreview)
            {
                m_currentPreview->detach(m_world);
                m_currentPreview.reset();
            }

            m_currentPreview = m_loadingPreview;

            if (m_positionUnderCursorValid && m_currentPreview)
                m_currentPreview->move(m_positionUnderCursor);

            m_currentPreview->attach(m_world);
        }

        if (m_positionUnderCursorValid && m_currentPreview)
            m_currentPreview->move(m_positionUnderCursor);
    }

    void render(ScenePreviewPanel* panel, rendering::FrameParams& frame)
    {
        if (m_positionUnderCursorValid)
        {
            rendering::DebugDrawer dd(frame.geometry.solid);
            dd.color(Color::CYAN);
            dd.solidSphere(m_positionUnderCursor, 0.05f);
        }
    }

private:
    RefPtr<ui::RenderingPanelDepthBufferQuery> m_depthBuffer;

    SceneGridSettings m_gridSettings;

    mutable ExactPosition m_positionUnderCursor;
    mutable bool m_positionUnderCursorValid = false;

    RefPtr<SceneObjectDragDropPreview> m_loadingPreview;
    RefPtr<SceneObjectDragDropPreview> m_currentPreview;

    World* m_world = nullptr;

    TCreateFunc m_createFunc;
};

//--

ui::DragDropHandlerPtr SceneEditMode_Default::handleDragDrop(ScenePreviewPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& absolutePosition, const Point& clientPosition)
{
    // we need target when a new node will be added
    auto activeNode = m_activeNode.lock();
    if (!activeNode)
        return nullptr; 

    // target list
    Array<SceneContentNodePtr> targetNodes;
    targetNodes.pushBack(activeNode);

    // file
    RefPtr<SceneObjectDragDropPreview> previewObject;
    SceneObjectDragDropCreationHandler::TCreateFunc createFunc;
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
    {
        const auto depotPath = fileData->depotPath();
        if (activeNode->canAttach(SceneContentNodeType::Entity))
        {
            if (CanLoadAsClass<Prefab>(depotPath))
            {
                createFunc = [this, depotPath, targetNodes](const Transform& placement)
                {
                    createPrefabAtNodes(targetNodes, depotPath, &placement);
                };

                previewObject = RefNew<SceneObjectDragDropPreview>();
                RunFiber("LoadObjectPreview") << [previewObject, depotPath](FIBER_FUNC)
                {
                    previewObject->buildPrefabPreview(depotPath);
                };
            }
            else if (auto resoureClass = LoadClass(depotPath))
            {
                if (auto entityClass = m_objectPalette->selectedEntityClass(resoureClass))
                {
                    createFunc = [this, depotPath, targetNodes, entityClass](const Transform& placement)
                    {
                        createEntityAtNodes(targetNodes, entityClass, &placement, depotPath);
                    };
                };
            }
        }
    }

    // nothing to create
    if (!createFunc)
        return nullptr;

    // capture depth buffer from whole panel
    auto depthBuffer = panel->queryDepth(nullptr); 
    if (!depthBuffer)
        return nullptr;        

    // create a drag&drop handler
    auto handler = RefNew<SceneObjectDragDropCreationHandler>(data, panel, absolutePosition, depthBuffer, 
        container()->world(), container()->gridSettings(), 
        createFunc, previewObject);
    m_dragDropHandler = handler;
    return handler;
}

void SceneEditMode_Default::updateDragDrop()
{
    if (auto handler = m_dragDropHandler.lock())
        handler->update();
}

void SceneEditMode_Default::renderDragDrop(ScenePreviewPanel* panel, rendering::FrameParams& frame)
{
    if (auto handler = m_dragDropHandler.lock())
        handler->render(panel, frame);
}

//--

END_BOOMER_NAMESPACE_EX(ed)

