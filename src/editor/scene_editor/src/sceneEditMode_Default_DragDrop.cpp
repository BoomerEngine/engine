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

#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiClassPickerBox.h"
#include "base/ui/include/uiRenderer.h"
#include "base/object/include/actionHistory.h"
#include "base/object/include/action.h"
#include "base/world/include/world.h"
#include "base/world/include/worldEntity.h"
#include "base/world/include/worldEntityBehavior.h"
#include "base/resource/include/objectIndirectTemplate.h"
#include "base/resource/include/objectIndirectTemplateCompiler.h"
#include "base/world/include/worldNodeTemplate.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedDirectory.h"
#include "base/world/include/worldPrefab.h"
#include "editor/common/include/editorService.h"
#include "editor/common/src/assetBrowserDialogs.h"
#include "editor/common/include/assetBrowser.h"
#include "base/ui/include/uiDragDrop.h"

#include "rendering/scene/include/renderingFrameDebug.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

extern void BindResourceToObjectTemplate(ObjectIndirectTemplate* ptr, const ManagedFile* file);

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

    void buildEntityPreview(ClassType entityClass, const ManagedFile* file)
    {
        auto compTemplate = RefNew<ObjectIndirectTemplate>();
        compTemplate->templateClass(entityClass);
        BindResourceToObjectTemplate(compTemplate, file);

        ObjectIndirectTemplateCompiler compiler;
        compiler.addTemplate(compTemplate);

        if (auto finalEntityClass = compiler.compileClass().cast<world::Entity>())
        {
            auto defaultEntity = (world::Entity*)finalEntityClass->defaultObject();
            finalEntityClass = defaultEntity->determineEntityTemplateClass(compiler);
            if (finalEntityClass)
            {
                if (auto entity = finalEntityClass->create<world::Entity>())
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

    void buildPrefabPreview(const ManagedFile* file)
    {
        if (const auto prefab = LoadResource<world::Prefab>(file->depotPath()))
        {
            const auto& rootPlacement = AbsoluteTransform::ROOT();

            InplaceArray<world::EntityPtr, 20> allEntities;
            prefab->compile(StringID(), rootPlacement, allEntities);

            for (const auto& ent : allEntities)
            {
                auto& info = m_entities.emplaceBack();
                info.entity = ent;
                info.relativePlacementTransform = ent->absoluteTransform() / rootPlacement;
            }
        }           

        m_ready.exchange(true);            
    }


    void move(const AbsoluteTransform& placement)
    {
        m_placement = placement;

        for (const auto& ent : m_entities)
        {
            auto entityPlacement = placement * ent.relativePlacementTransform;
            ent.entity->requestTransform(entityPlacement);
        }
    }

    void attach(world::World* world)
    {
        for (const auto& ent : m_entities)
            world->attachEntity(ent.entity);
    }

    void detach(world::World* world)
    {
        for (const auto& ent : m_entities)
            world->detachEntity(ent.entity);
    }
        
private:
    AbsoluteTransform m_placement;

    struct Entity
    {
        world::EntityPtr entity;
        Transform relativePlacementTransform;
    };

    Array<Entity> m_entities;

    std::atomic<bool> m_ready = false;
};

//--

class SceneObjectDragDropCreationHandler : public ui::IDragDropHandler
{
public:
    typedef std::function<void(const base::AbsoluteTransform&)> TCreateFunc;

    SceneObjectDragDropCreationHandler(const ui::DragDropDataPtr& data, ui::IElement* target, const ui::Position& initialPosition, ui::RenderingPanelDepthBufferQuery* depthData, world::World* world, const SceneGridSettings& gridSettings, const TCreateFunc& createFunc, SceneObjectDragDropPreview* preview)
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
        base::Point clientPos = absolutePos - target()->cachedDrawArea().absolutePosition();

        base::AbsolutePosition worldPos;
        if (m_depthBuffer->calcWorldPosition(clientPos.x, clientPos.y, worldPos))
        {
            if (m_gridSettings.positionGridEnabled)
            {
                double x, y, z;
                worldPos.expand(x, y, z);

                auto snappedX = Snap(x, m_gridSettings.positionGridSize);
                auto snappedY = Snap(y, m_gridSettings.positionGridSize);
                auto snappedZ = Snap(z, m_gridSettings.positionGridSize);
                m_positionUnderCursor = base::AbsolutePosition(snappedX, snappedY, snappedZ);
            }
            else
            {
                m_positionUnderCursor = worldPos;
            }

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

    void render(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
    {
        if (m_positionUnderCursorValid)
        {
            rendering::scene::DebugDrawer dd(frame.geometry.solid);
            dd.color(base::Color::CYAN);
            dd.solidSphere(m_positionUnderCursor.approximate(), 0.05f);
        }
    }

private:
    base::RefPtr<ui::RenderingPanelDepthBufferQuery> m_depthBuffer;

    SceneGridSettings m_gridSettings;

    mutable base::AbsolutePosition m_positionUnderCursor;
    mutable bool m_positionUnderCursorValid = false;

    RefPtr<SceneObjectDragDropPreview> m_loadingPreview;
    RefPtr<SceneObjectDragDropPreview> m_currentPreview;

    world::World* m_world = nullptr;

    TCreateFunc m_createFunc;
};

//--

ui::DragDropHandlerPtr SceneEditMode_Default::handleDragDrop(ScenePreviewPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& absolutePosition, const base::Point& clientPosition)
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
    if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
    {
        if (auto file = fileData->file())
        {
            const auto resClass = file->fileFormat().nativeResourceClass();
            if (activeNode->canAttach(SceneContentNodeType::Entity))
            {
                if (resClass.is<world::Prefab>())
                {
                    createFunc = [this, file, targetNodes](const base::AbsoluteTransform& placement)
                    {
                        createPrefabAtNodes(targetNodes, file, &placement);
                    };

                    previewObject = RefNew<SceneObjectDragDropPreview>();
                    RunFiber("LoadObjectPreview") << [previewObject, file](FIBER_FUNC)
                    {
                        previewObject->buildPrefabPreview(file);
                    };
                }
                else
                {
                    if (auto entityClass = m_objectPalette->selectedEntityClass(resClass))
                    {
                        createFunc = [this, file, targetNodes, entityClass](const base::AbsoluteTransform& placement)
                        {
                            createEntityAtNodes(targetNodes, entityClass, &placement, file);
                        };
                    };
                }
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

void SceneEditMode_Default::renderDragDrop(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
{
    if (auto handler = m_dragDropHandler.lock())
        handler->render(panel, frame);
}

//--

END_BOOMER_NAMESPACE(ed)

