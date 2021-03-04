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

#include "engine/world/include/entity.h"
#include "engine/world/include/entityBehavior.h"
#include "engine/world/include/nodeTemplate.h"
#include "engine/world/include/prefab.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedDirectory.h"
#include "editor/common/include/editorService.h"
#include "editor/common/src/assetBrowserDialogs.h"
#include "editor/common/include/managedDepot.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiClassPickerBox.h"
#include "engine/ui/include/uiRenderer.h"
#include "core/object/include/actionHistory.h"
#include "core/object/include/action.h"
#include "core/resource/include/indirectTemplate.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

void SceneEditMode_Default::buildContextMenu(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    // HACK: reset global context state
    if (setup.contextWorldPositionValid)
    {
        m_contextMenuPlacementTransformValid = true;
        m_contextMenuPlacementTransform = setup.contextWorldPosition;
    }
    else
    {
        m_contextMenuPlacementTransformValid = false;
        m_contextMenuPlacementTransform = AbsoluteTransform();
    }

    if (setup.viewportBased)
    {
        m_contextMenuContextNodes.reset();
        if (auto active = m_activeNode.lock())
            m_contextMenuContextNodes.pushBack(active);
    }
    else if (setup.contextTreeItem)
    {
        m_contextMenuContextNodes = setup.selection;
    }
    else
    {
        m_contextMenuContextNodes.reset();
    }

    //--       

    buildContextMenu_Focus(menu, setup);
    menu->createSeparator();

    buildContextMenu_Create(menu, setup);
    menu->createSeparator();

    buildContextMenu_ShowHide(menu, setup);
    menu->createSeparator();

    buildContextMenu_Clipboard(menu, setup);
    menu->createSeparator();

    buildContextMenu_Prefab(menu, setup);
    menu->createSeparator();

    buildContextMenu_Resources(menu, setup);
    menu->createSeparator();

    buildContextMenu_ContextNode(menu, setup);
    menu->createSeparator();

    //--
}

//--

void SceneEditMode_Default::buildContextMenu_Focus(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    if (setup.viewportBased)
    {
        if (setup.contextClickedItem)
        {
            if (auto dataNode = rtti_cast<SceneContentDataNode>(setup.contextClickedItem))
            {
                menu->createCallback(TempString("Focus on '{}'", dataNode->name()), "[img:zoom]", "F") = [this, setup]()
                {
                    focusNode(setup.contextClickedItem);
                };
            }
        }
    }
    else if (setup.contextTreeItem)
    {
        if (auto dataNode = rtti_cast<SceneContentDataNode>(setup.contextTreeItem))
        {
            menu->createCallback(TempString("Focus on '{}'", dataNode->name()), "[img:zoom]", "F") = [this, setup]()
            {
                focusNode(setup.contextTreeItem);
            };
        }
    }
}

void SceneEditMode_Default::buildContextMenu_ShowHide(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    bool canShow = false;
    bool canHide = false;
    for (const auto& node : setup.selection)
    {
        if (node->visibilityFlagBool())
            canHide = true;
        else
            canShow = true;
    }

    if (canShow)
    {
        menu->createCallback("Show", "[img:eye]", "") = [this, setup]()
        {
            processObjectShow(setup.selection);
        };
    }

    if (canHide)
    {
        menu->createCallback("Hide", "[img:eye_cross]", "") = [this, setup]()
        {
            processObjectHide(setup.selection);
        };
    }

}

void SceneEditMode_Default::buildContextMenu_Create(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    // active file
    const auto* activeFile = GetEditor()->selectedFile();
    const auto activeFileResourceClass = activeFile ? activeFile->fileFormat().nativeResourceClass() : nullptr;

    // "add" 
    bool canAddDir = false;
    bool canAddLayer = false;
    bool canAddEntity = false;
    bool canAddEmptyEntity = false;
    bool canAddBehavior = false;
    for (const auto& node : m_contextMenuContextNodes)
    {
        if (node->canAttach(SceneContentNodeType::Entity))
        {
            canAddEntity = true;

            if (node->type() == SceneContentNodeType::Entity)
                canAddEmptyEntity = true;
        }

        if (node->canAttach(SceneContentNodeType::Behavior))
            canAddBehavior = true;

        if (node->canAttach(SceneContentNodeType::LayerFile))
            canAddLayer = true;

        if (node->canAttach(SceneContentNodeType::LayerDir))
            canAddDir = true;
    }

    /*if (canAddEmptyEntity)
    {
        menu->createCallback("Add child", "[img:add]") = [this, setup]()
        {
            createEntityAtNodes(setup.selection, nullptr);
        };
    }*/

    if (canAddEntity)
    {
        if (activeFile)
        {
            if (activeFileResourceClass.is<Prefab>())
            {
                menu->createCallback(TempString("Add prefab '{}'", activeFile->name().view().fileStem()), "[img:add]") = [this, activeFile]()
                {
                    createPrefabAtNodes(m_contextMenuContextNodes, activeFile);
                };
            }
            else
            {
                if (auto entityClass = m_objectPalette->selectedEntityClass(activeFileResourceClass))
                {
                    menu->createCallback(TempString("Add {} using '{}'", entityClass, activeFile->name().view().fileStem()), "[img:add]") = [this, activeFile, entityClass]()
                    {
                        const auto* placement = m_contextMenuPlacementTransformValid ? &m_contextMenuPlacementTransform : nullptr;
                        createEntityAtNodes(m_contextMenuContextNodes, entityClass, placement, activeFile);
                    };
                }
            }
        }

        menu->createSubMenu(m_entityClassSelector, "Add entity", "[img:add]");
    }

    if (canAddBehavior)
    {
        menu->createSubMenu(m_behaviorClassSelector, "Add behavior", "[img:add]");
    }

    if (canAddLayer)
    {
        menu->createCallback("Add layer", "[img:file_add]") = [this]()
        {
            processCreateLayer(m_contextMenuContextNodes);
        };
    }

    if (canAddDir)
    {
        menu->createCallback("Add directory", "[img:folder_add]") = [this]()
        {
            processCreateDirectory(m_contextMenuContextNodes);
        };
    }
}

void SceneEditMode_Default::buildContextMenu_Clipboard(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    if (!setup.selection.empty())
    {
        bool canDelete = true;
        bool canCopy = true;
        bool onlyEntities = true;
        for (const auto& node : setup.selection)
        {
            canDelete &= node->canDelete();
            canCopy &= node->canCopy();

            if (node->type() != SceneContentNodeType::Entity)
                onlyEntities = false;
        }

        // copy
        menu->createCallback("Copy", "[img:copy]", "Ctrl+C", canCopy) = [this, setup]()
        {
            processObjectCopy(setup.selection);
        };

        // cut
        menu->createCallback("Cut", "[img:cut]", "Ctrl+X", canCopy && canDelete) = [this, setup]()
        {
            processObjectCut(setup.selection);
        };

        // delete
        menu->createCallback("Delete", "[img:delete]", "Delete", canDelete) = [this, setup]()
        {
            processObjectDeletion(setup.selection);
        };

        // duplicate
        menu->createCallback("Duplicate", "[img:page_copy]", "Ctrl+D", canCopy) = [this, setup]()
        {
            processObjectDuplicate(setup.selection);
        };
    }

    // paste
    if (m_panel->renderer())
    {
        SceneContentClipboardDataPtr data;
        if (m_panel->renderer()->loadObjectFromClipboard(data) && !data->data.empty())
        {
            StringBuilder caption;
            caption << "Paste ";

            if (setup.contextWorldPositionValid)
                caption << "here ";

            const auto activeNode = m_activeNode.lock();

            if (setup.contextTreeItem)
            {
                if (setup.contextTreeItem->canAttach(data->type))
                {
                    auto pasteMode = SceneContentNodePasteMode::Relative;
                    menu->createCallback(caption.toString(), "[img:paste]", "Ctrl+V") = [this, setup, data, pasteMode]()
                    {
                        processObjectPaste(setup.contextTreeItem, data, pasteMode);
                    };
                }
            }
            else if (setup.contextWorldPositionValid && activeNode)
            {
                if (activeNode->canAttach(data->type))
                {
                    auto pasteMode = SceneContentNodePasteMode::Absolute;
                    menu->createCallback(caption.toString(), "[img:paste]", "") = [this, setup, activeNode, data, pasteMode]()
                    {
                        processObjectPaste(activeNode, data, pasteMode, &m_contextMenuPlacementTransform);
                    };
                }
            }
        }
    }
}

void SceneEditMode_Default::buildContextMenu_Prefab(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    if (!setup.selection.empty())
    {
        bool onlyEntities = true;
        bool explodable = true;
        for (const auto& node : setup.selection)
        {
            if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
            {
                explodable &= entityNode->canExplodePrefab();
            }
            else
            {
                onlyEntities = false;
            }
        }

        if (onlyEntities)
        {
            menu->createCallback("Save as prefab...", "[img:save]", "") = [this, setup]()
            {
                processSaveAsPrefab(setup.selection);
            };
        }

        if (explodable)
        {
            menu->createSeparator();

            menu->createCallback("Unpack prefab", "[img:package_go]", "") = [this, setup]()
            {
                processUnwrapPrefab(setup.selection, false);
            };

            menu->createCallback("Explode prefab", "[img:bomb]", "") = [this, setup]()
            {
                processUnwrapPrefab(setup.selection, true);
            };
        }
    }
}

void SceneEditMode_Default::buildContextMenu_ContextNode(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    if (setup.contextClickedItem)
    {
        InplaceArray<SceneContentNodePtr, 10> possibleActiveRoots;
        {
            const auto* node = setup.contextClickedItem.get();
            while (node)
            {
                if (node->type() != SceneContentNodeType::Entity && node->type() != SceneContentNodeType::Behavior && node->type() != SceneContentNodeType::LayerFile)
                    break;
                possibleActiveRoots.pushBack(AddRef(node));
                node = node->parent();
            }
        }

        //auto subMenu = RefNew<ui::MenuButtonContainer>();
        for (auto i : possibleActiveRoots.indexRange().reversed())
        {
            auto node = possibleActiveRoots[i];

            StringBuilder caption;
            caption << "Activate ";
            caption << SceneContentNode::IconTextForType(node->type());
            caption << " ";
            caption << node->buildHierarchicalName();

            const auto alreadyActive = (m_activeNode == node);
            menu->createCallback(caption.toString(), alreadyActive ? "[img:star]" : "", "") = [this, node]()
            {
                this->activeNode(node);
            };
        }

    }
    else if (setup.contextTreeItem)
    {
        const auto alreadyActive = (m_activeNode == setup.contextTreeItem);
        menu->createCallback("Make active", alreadyActive ? "[img:star_gray]" : "[img:star]", "", !alreadyActive) = [this, setup]()
        {
            activeNode(setup.contextTreeItem);
        };
    }
}

static void ExtractResourcesFromNode(const SceneContentNode* node, HashMap<ResourcePath, uint32_t>& outResources)
{
    if (node->type() != SceneContentNodeType::Entity && node->type() != SceneContentNodeType::Behavior)
        return;

    if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
        if (auto data = entityNode->compileSnapshot())
            ExtractUsedResources(data, outResources);

    for (const auto& child : node->children())
        ExtractResourcesFromNode(child, outResources);
}

void SceneEditMode_Default::buildContextMenu_Resources(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup)
{
    HashMap<ResourcePath, uint32_t> usedResources;

    if (setup.contextClickedItem)
        ExtractResourcesFromNode(setup.contextClickedItem, usedResources);
    else if (setup.contextTreeItem)
        ExtractResourcesFromNode(setup.contextTreeItem, usedResources);

    if (!usedResources.empty())
    {
        struct Entry
        {
            ResourcePath key;
            uint32_t count = 0;
        };

        InplaceArray<Entry, 32> tempPairs;

        for (auto pair : usedResources.pairs())
        {
            auto& info = tempPairs.emplaceBack();
            info.key = pair.key;
            info.count = pair.value;
        }

        std::sort(tempPairs.begin(), tempPairs.end(), [](const Entry& a, const Entry& b) { return a.count < b.count; });

        auto subMenu = RefNew<ui::MenuButtonContainer>();

        for (const auto& pair : tempPairs)
        {
            if (auto* file = GetEditor()->managedDepot().findManagedFile(pair.key.view()))
            {
                StringBuilder txt;
                file->fileFormat().printTags(txt, " ");
                txt << file->depotPath();

                subMenu->createCallback(txt.toString(), "[img:page_up]", "") = [this, file]()
                {
                    GetEditor()->showFile(file);
                };
            }
        }

        menu->createSubMenu(subMenu->convertToPopup(), "Resources", "[img:page_zoom]");                
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)

