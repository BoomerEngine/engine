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
#include "sceneContentStructure.h"
#include "sceneEditMode_Default.h"

#include "engine/world/include/prefab.h"
#include "engine/world/include/rawScene.h"
#include "engine/world/include/rawLayer.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileNativeResource.h"
#include "editor/common/include/managedItem.h"
#include "editor/common/include/managedDirectory.h"
#include "editor/common/include/editorService.h"
#include "core/io/include/io.h"
#include "engine/ui/include/uiToolBar.h"

#undef DeleteFile

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneWorldEditor);
RTTI_END_TYPE();

SceneWorldEditor::SceneWorldEditor(ManagedFileNativeResource* file)
    : SceneCommonEditor(file, SceneContentNodeType::WorldRoot, "Main")
{
    m_rootLayersGroup = RefNew<SceneContentWorldDir>("layers", true);
    m_content->root()->attachChildNode(m_rootLayersGroup);
    m_content->root()->resetModifiedStatus(true);

    toolbar()->createCallback(ui::ToolbarButtonSetup().caption("Assets").icon("database").tooltip("Show asset browser window")) = [this]()
    {
        cmdShowAssetBrowser();
    };
}

SceneWorldEditor::~SceneWorldEditor()
{}

void SceneWorldEditor::cmdShowAssetBrowser()
{
    GetEditor()->showAssetBrowser();
}

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

static void ExtractLayerStructure(const ManagedDirectory* dir, SceneContentNode* dirNode, Array<StringBuf>& outErrors)
{
    for (const auto* file : dir->files())
    {
        if (file->fileFormat().loadableAsType(RawLayer::GetStaticClass()))
        {
            if (const auto* nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            {
                if (const auto layerData = rtti_cast<RawLayer>(nativeFile->loadContent()))
                {
                    if (const auto layerNode = UnpackLayer(file->name().view().beforeFirst("."), layerData, outErrors))
                    {
                        dirNode->attachChildNode(layerNode);
                    }
                    else
                    {
                        outErrors.emplaceBack(TempString("Failed to unpack content of layer '{}'", nativeFile->depotPath()));
                    }
                }
                else
                {
                    outErrors.emplaceBack(TempString("Failed to load content for layer '{}'", nativeFile->depotPath()));
                }
            }
        }
    }

    for (const auto* childDir : dir->directories())
    {
        auto childDirNode = RefNew<SceneContentWorldDir>(childDir->name(), false);
        ExtractLayerStructure(childDir, childDirNode, outErrors);
        dirNode->attachChildNode(childDirNode);
    }
}

void SceneWorldEditor::recreateContent()
{
    // layers are stored in "layers" directory
    if (auto* layersDirectory = nativeFile()->parentDirectory()->createDirectory("layers"))
    {
        Array<StringBuf> reportedErrors;
        ExtractLayerStructure(layersDirectory, m_rootLayersGroup, reportedErrors);

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
    Array<NodeTemplatePtr> nodes;
    nodes.reserve(data->entities().size());

    for (const auto& content : data->entities())
        if (auto node = content->compileDifferentialData())
            nodes.pushBack(node);

    auto ret = RefNew<RawLayer>();
    ret->setup(nodes);
    return ret;
}

static void CollectDirToDelete(ManagedDirectory* dir, Array<ManagedDirectory*>& outDirsToDelete, Array<ManagedFile*>& outFilesToDelete)
{
    for (auto* childDir : dir->directories())
        CollectDirToDelete(childDir, outDirsToDelete, outFilesToDelete);

    for (auto* file : dir->files())
        outFilesToDelete.pushBack(file);

    outDirsToDelete.pushBack(dir);
}

static void SaveLayerStructure(ManagedDirectory* dir, const SceneContentWorldDir* data, Array<ManagedDirectory*>& outDirsToDelete, Array<ManagedFile*>& outFilesToDelete, Array<StringBuf>& outErrors)
{
    auto existingFiles = dir->files(); // copy
    auto existingDirs = dir->directories(); // copy
    for (const auto& child : data->children())
    {
        if (auto childDataDir = rtti_cast<SceneContentWorldDir>(child))
        {
            if (auto* childDir = dir->createDirectory(childDataDir->name()))
            {
                existingDirs.remove(childDir);
                SaveLayerStructure(childDir, childDataDir, outDirsToDelete, outFilesToDelete, outErrors);
            }
            else
            {
                outErrors.emplaceBack(TempString("Failed to create directory '{}' in '{}'", childDataDir->name(), dir->depotPath()));
            }
        }
        else if (auto childDataLayer = rtti_cast<SceneContentWorldLayer>(child))
        {
            bool shouldSave = childDataLayer->modified();

            auto layerExt = IResource::GetResourceExtensionForClass(RawLayer::GetStaticClass());

            auto fullFileName = StringBuf(TempString("{}.{}", childDataLayer->name(), layerExt));
            auto* existingFile = rtti_cast<ManagedFileNativeResource>(dir->file(fullFileName, true));
            shouldSave |= (existingFile == nullptr) || (existingFile->isDeleted());

            if (existingFile)
                existingFiles.remove(existingFile);

            if (shouldSave)
            {
                if (auto layerDataToSave = BuildLayerData(childDataLayer))
                {
                    if (existingFile)
                    {
                        if (!existingFile->storeContent(layerDataToSave))
                        {
                            outErrors.emplaceBack(TempString("Failed to save layer '{}' in '{}'", childDataDir->name(), dir->depotPath()));
                        }
                        else
                        {
                            childDataLayer->resetModifiedStatus(true);
                        }
                    }
                    else
                    {
                        if (!dir->createFile(childDataLayer->name(), layerDataToSave))
                        {
                            outErrors.emplaceBack(TempString("Failed to save layer '{}' in '{}'", childDataDir->name(), dir->depotPath()));
                        }
                        else
                        {
                            childDataLayer->resetModifiedStatus(true);
                        }
                    }
                }
                else
                {
                    outErrors.emplaceBack(TempString("Failed to save nodes to layer '{}' in '{}'", childDataDir->name(), dir->depotPath()));
                }
            }
        }
    }

    // anything existing that was not saved we need to delete
    for (auto* dir : existingDirs)
        CollectDirToDelete(dir, outDirsToDelete, outFilesToDelete);
    for (auto* file : existingFiles)
        outFilesToDelete.pushBack(file);
}

bool SceneWorldEditor::save()
{
    // save scene file only if it's modified
    if (nativeFile()->isModified())
        if (!TBaseClass::save())
            return false;

    // create the "layers" directory
    auto* layersDirectory = nativeFile()->parentDirectory()->createDirectory("layers");
    if (!layersDirectory)
        return false;

    // save the directory/layer structure
    Array<ManagedDirectory*> dirsToDelete;
    Array<ManagedFile*> filesToDelete;
    Array<StringBuf> reportedErrors;
    SaveLayerStructure(layersDirectory, m_rootLayersGroup, dirsToDelete, filesToDelete, reportedErrors);

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
    for (auto* file : filesToDelete)
        DeleteFile(file->absolutePath());
    for (auto* dir : dirsToDelete)
        DeleteDir(dir->absolutePath());

    // mark as layers as saved
    m_content->root()->resetModifiedStatus(true);
    return true;
}

//---

class SceneWorldResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool canOpen(const ManagedFileFormat& format) const override
    {
        return format.nativeResourceClass() == RawScene::GetStaticClass();
    }

    virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            return RefNew<SceneWorldEditor>(nativeFile);

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneWorldResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
