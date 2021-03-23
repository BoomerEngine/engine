/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"

#include "sceneWorldEditor.h"
#include "sceneContentNodes.h"
#include "sceneContentNodesEntity.h"
#include "sceneContentStructure.h"
#include "sceneWorldPlayerInEditor.h"
#include "sceneEditMode_Default.h"

#include "engine/world/include/rawPrefab.h"
#include "engine/world/include/rawWorldData.h"
#include "engine/world/include/rawLayer.h"
#include "engine/ui/include/uiToolBar.h"

#include "core/io/include/io.h"
#include "core/resource/include/depot.h"
#include "core/app/include/projectSettingsService.h"

#include "engine/ui/include/uiDockLayout.h"
#include "engine/world_compiler/include/worldCompiler.h"

#undef DeleteFile

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneWorldEditor);
RTTI_END_TYPE();

SceneWorldEditor::SceneWorldEditor(const ResourceInfo& info)
    : SceneCommonEditor(info, SceneContentNodeType::WorldRoot, "Main")
{
    m_rootLayersGroup = RefNew<SceneContentWorldDir>("layers", true);
    m_content->root()->attachChildNode(m_rootLayersGroup);
    m_content->root()->resetModifiedStatus(true);

    recreateContent();
    refreshEditMode();

    {
        m_piePanel = RefNew<ScenePlayInEditorPanel>(info.resourceDepotPath, m_content, m_previewContainer);
        dockLayout().attachPanel(m_piePanel, false);
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo().caption("Build", "cog")) = [this]() { cmdBuildWorld(); };
    }
}

SceneWorldEditor::~SceneWorldEditor()
{}

static RefPtr<SceneContentWorldLayer> UnpackLayer(StringView name, const RawLayer* layer, Array<StringBuf>& outErrors)
{
    auto ret = RefNew<SceneContentWorldLayer>(StringBuf(name));

    for (const auto& node : layer->nodes())
    {
        auto dataNode = RefNew<SceneContentEntityNode>(StringBuf(node->m_name.view()), node);
        ret->attachChildNode(dataNode);
    }

    return ret;
}

struct ExtractedLayer
{
    SceneContentNode* dirNode = nullptr;
    StringBuf path;
};

static void ExtractLayerStructure(StringView depotPath, SceneContentNode* dirNode, Array<ExtractedLayer>& outLayerPaths)
{
    GetService<DepotService>()->enumFilesAtPath(depotPath, [depotPath, dirNode, &outLayerPaths](StringView name)
        {
            if (name.endsWith(RawLayer::FILE_EXTENSION))
            {
                auto& info = outLayerPaths.emplaceBack();
                info.path = TempString("{}{}", depotPath, name);
                info.dirNode = dirNode;
            }
        });

    GetService<DepotService>()->enumDirectoriesAtPath(depotPath, [depotPath, dirNode, &outLayerPaths](StringView name)
        {
            auto childDirNode = RefNew<SceneContentWorldDir>(StringBuf(name), false);
            ExtractLayerStructure(TempString("{}{}/", depotPath, name), childDirNode, outLayerPaths);
            dirNode->attachChildNode(childDirNode);
        });
}

void SceneWorldEditor::recreateContent()
{
    // get the path to layers directory
    const auto layersPath = StringBuf(TempString("{}layers/", info().resourceDepotPath.view().baseDirectory()));

    // find layers
    Array<ExtractedLayer> layerPaths;
    ExtractLayerStructure(layersPath, m_rootLayersGroup, layerPaths);

    // load layers
    Array<StringBuf> reportedErrors;
    for (const auto& layerPath : layerPaths)
    {
        const auto name = layerPath.path.view().fileStem();
        if (const auto layerData = GetService<DepotService>()->loadFileToResource<RawLayer>(layerPath.path))
        {
            if (const auto layerNode = UnpackLayer(name, layerData, reportedErrors))
                layerPath.dirNode->attachChildNode(layerNode);
            else
                reportedErrors.emplaceBack(TempString("Failed to unpack content of layer '{}'", name));
        }
        else
        {
            reportedErrors.emplaceBack(TempString("Failed to load content for layer '{}'", name));
        }
    }


    // show loading errors
    if (!reportedErrors.empty())
    {
        StringBuilder txt;
        txt << "Scene loading errors:[br]";
        auto numErrors = std::min<uint32_t>(reportedErrors.size(), 100);
        for (uint32_t i = 0; i < numErrors; ++i)
        {
            txt << reportedErrors[i];
            txt << "[br]";
        }

        ui::PostNotificationMessage(this, ui::MessageType::Error, "LoadSave"_id, txt.view());
    }

    // reset all modified state after loading
    m_content->root()->resetModifiedStatus();
}

bool SceneWorldEditor::checkGeneralSave() const
{
    if (const auto& root = m_content->root())
        return root->modified();

    return false;
}

static RawLayerPtr BuildLayerData(const SceneContentWorldLayer* data)
{
    Array<RawEntityPtr> nodes;
    nodes.reserve(data->entities().size());

    for (const auto& content : data->entities())
        if (auto node = content->compileDifferentialData())
            nodes.pushBack(node);

    auto ret = RefNew<RawLayer>();
    ret->setup(nodes);
    return ret;
}

static void CollectDirToDelete(StringView dirPath, Array<StringBuf>& outDirsToDelete, Array<StringBuf>& outFilesToDelete)
{
    GetService<DepotService>()->enumDirectoriesAtPath(dirPath, [dirPath, &outDirsToDelete, &outFilesToDelete](StringView name)
        {CollectDirToDelete(TempString("{}{}/", dirPath, name), outDirsToDelete, outFilesToDelete); });

    GetService<DepotService>()->enumFilesAtPath(dirPath, [dirPath, outDirsToDelete, &outFilesToDelete](StringView name)
        {outFilesToDelete.emplaceBack(TempString("{}{}", dirPath, name)); });

    outDirsToDelete.emplaceBack(dirPath);
}

static void SaveLayerStructure(StringView dirPath, const SceneContentWorldDir* data, Array<StringBuf>& outDirsToDelete, Array<StringBuf>& outFilesToDelete, Array<StringBuf>& outErrors)
{
    Array<StringBuf> existingDirNames;
    GetService<DepotService>()->enumDirectoriesAtPath(dirPath, [dirPath, &existingDirNames](StringView name)
        {existingDirNames.emplaceBack(name); });

    Array<StringBuf> existingFileNames;
    GetService<DepotService>()->enumFilesAtPath(dirPath, [outDirsToDelete, &existingFileNames](StringView name)
        { 
            if (name.endsWith(RawLayer::FILE_EXTENSION))
                existingFileNames.emplaceBack(name.fileStem()); 
        });
    
    for (const auto& child : data->children())
    {
        if (auto childDataDir = rtti_cast<SceneContentWorldDir>(child))
        {
            const auto childDepotDir = StringBuf(TempString("{}{}/", dirPath, childDataDir->name()));
            if (GetService<DepotService>()->createDirectories(childDepotDir))
            {
                existingDirNames.remove(childDataDir->name());
                SaveLayerStructure(childDepotDir, childDataDir, outDirsToDelete, outFilesToDelete, outErrors);
            }
            else
            {
                outErrors.emplaceBack(TempString("Failed to create directory '{}'", childDataDir->name(), childDepotDir));
            }
        }
        else if (auto childDataLayer = rtti_cast<SceneContentWorldLayer>(child))
        {
            auto fullFileName = StringBuf(TempString("{}{}.{}", dirPath, childDataLayer->name(), RawLayer::FILE_EXTENSION));
            bool fileExists = GetService<DepotService>()->fileExists(fullFileName);

            if (fileExists)
                existingFileNames.remove(childDataLayer->name());

            if (childDataLayer->modified() || !fileExists)
            {
                if (auto layerDataToSave = BuildLayerData(childDataLayer))
                {
                    if (GetService<DepotService>()->saveFileFromResource(fullFileName, layerDataToSave))
                        childDataLayer->resetModifiedStatus(true);
                    else
                        outErrors.emplaceBack(TempString("Failed to save layer '{}'", fullFileName));
                }
                else
                {
                    outErrors.emplaceBack(TempString("Failed to save nodes to layer '{}'", fullFileName));
                }
            }
        }
    }

    // anything existing that was not saved we need to delete
    for (const auto& dirName : existingDirNames)
        CollectDirToDelete(TempString("{}{}/", dirPath, dirName), outDirsToDelete, outFilesToDelete);
    for (const auto& fileName : existingFileNames)
        outFilesToDelete.emplaceBack(TempString("{}{}.{}", dirPath, fileName, RawLayer::FILE_EXTENSION));
}

bool SceneWorldEditor::save()
{
    // save scene file only if it's modified
    if (info().resource->modified())
        if (!TBaseClass::save())
            return false;

    // get the path to layers directory
    const auto layersPath = StringBuf(TempString("{}layers/", info().resourceDepotPath.view().baseDirectory()));

    // save the directory/layer structure
    Array<StringBuf> dirsToDelete;
    Array<StringBuf> filesToDelete;
    Array<StringBuf> reportedErrors;
    SaveLayerStructure(layersPath, m_rootLayersGroup, dirsToDelete, filesToDelete, reportedErrors);

    // display errors
    if (!reportedErrors.empty())
    {
        StringBuilder txt;
        txt << "Scene saving errors:[br]";
        auto numErrors = std::min<uint32_t>(reportedErrors.size(), 100);
        for (uint32_t i = 0; i < numErrors; ++i)
        {
            txt << reportedErrors[i];
            txt << "[br]";
        }

        ui::PostNotificationMessage(this, ui::MessageType::Error, "LoadSave"_id, txt.view());
        return false;
    }

    // remove files that are no longer needed
    for (const auto& file : filesToDelete)
        GetService<DepotService>()->removeFile(file);
    for (const auto& dir : dirsToDelete)
        GetService<DepotService>()->removeDirectory(dir);

    // mark as layers as saved
    m_content->root()->resetModifiedStatus(true);
    return true;
}

void SceneWorldEditor::cleanup()
{
    TBaseClass::cleanup();
    m_piePanel->stopGame();
}

//---

void SceneWorldEditor::cmdBuildWorld()
{

}

//---

class SceneWorldResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const override final
    {
        if (auto world = rtti_cast<RawWorldData>(context.resource))
        {
            outEditor = RefNew<SceneWorldEditor>(context);
            return true;
        }

        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneWorldResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
