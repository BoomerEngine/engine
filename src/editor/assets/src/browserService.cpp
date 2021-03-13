/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "browserPanel.h"
#include "browserService.h"

#include "resourceContainer.h"
#include "resourceEditor.h"

#include "editor/common/include/service.h"
#include "engine/ui/include/uiElementConfig.h"
#include "core/containers/include/path.h"
#include "core/resource/include/metadata.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

RTTI_BEGIN_TYPE_CLASS(AssetBrowserService);
    RTTI_METADATA(DependsOnServiceMetadata).dependsOn<EditorService>();
RTTI_END_TYPE();

///--

AssetBrowserService::AssetBrowserService()
    : m_depotEvents(this)
{
}

AssetBrowserService::~AssetBrowserService()
{
}

//--

ui::ConfigBlock AssetBrowserService::config()
{
    return GetService<EditorService>()->config().tag("AssetBrowser");
}

bool AssetBrowserService::onInitializeService(const CommandLine& cmdLine)
{
    auto key = GetService<EditorService>()->eventKey();
    m_depotEvents.bind(key, EVENT_EDITOR_CONFIG_SAVE) 
        = [this]() { (config()); };

    configLoad(config());

    return true;
}

void AssetBrowserService::onShutdownService()
{
}

void AssetBrowserService::onSyncUpdate()
{
}

//--

void AssetBrowserService::configLoad(const ui::ConfigBlock& block)
{
    // load bookmarks
    {
        m_bookmarkedDirectories.clear();

        auto bookmarks = block.readOrDefault<Array<StringBuf>>("BookmarkedDirectories");
        for (const auto& path : bookmarks)
            m_bookmarkedDirectories.insert(path);
    }
}

void AssetBrowserService::configSave(const ui::ConfigBlock& block) const
{
    // save bookmarks
    block.write<Array<StringBuf>>("BookmarkedDirectories", m_bookmarkedDirectories.keys());
}

//--

bool AssetBrowserService::checkDirectoryBookmark(StringView depotPath) const
{
    return m_bookmarkedDirectories.contains(depotPath);
}

void AssetBrowserService::toggleDirectoryBookmark(StringView depotPath, bool state)
{
    bool changed = false;
    if (m_bookmarkedDirectories.contains(depotPath))
    {
        if (!state)
        {
            m_bookmarkedDirectories.remove(depotPath);
            changed = true;
        }
    }
    else
    {
        if (state)
        {
            m_bookmarkedDirectories.insert(StringBuf(depotPath));
            changed = true;
        }
    }

    if (changed)
    {
        DispatchGlobalEvent(eventKey(), EVENT_DEPOT_DIRECTORY_BOOKMARKED, StringBuf(depotPath));
    }
}

//--

StringBuf AssetBrowserService::selectedFile() const
{
    if (auto panel = GetService<EditorService>()->findPanel<AssetBrowserPanel>())
        return panel->selectedFile();
    return "";
}

StringBuf AssetBrowserService::currentDirectory() const
{
    if (auto panel = GetService<EditorService>()->findPanel<AssetBrowserPanel>())
        return panel->currentDirectory();
    return "";
}

void AssetBrowserService::showWindow(bool activate)
{
    if (auto panel = GetService<EditorService>()->findPanel<AssetBrowserPanel>())
    {
        if (auto window = panel->findParentWindow())
        {
            window->requestShow(activate);
        }
    }
}

void AssetBrowserService::showFile(StringView depotPath)
{
    if (auto panel = GetService<EditorService>()->findPanel<AssetBrowserPanel>())
        panel->showFile(depotPath);
}

void AssetBrowserService::showDirectory(StringView depotPath)
{
    if (auto panel = GetService<EditorService>()->findPanel<AssetBrowserPanel>())
        panel->showDirectory(depotPath);
}

//--

void AssetBrowserService::clearMarkedFiles()
{
    auto oldFiles = m_markedDepotFiles.keys();
    m_markedDepotFiles.clear();
    m_markedDepotFilesTag = 0;

    for (const auto& path : oldFiles)
        DispatchGlobalEvent(eventKey(), EVENT_DEPOT_ASSET_MARKED, path);
}

void AssetBrowserService::markFiles(const Array<StringBuf>& paths, int tag)
{
    clearMarkedFiles();

    if (tag)
    {
        m_markedDepotFilesTag = tag;

        for (const auto& path : paths)
            if (m_markedDepotFiles.insert(path))
                DispatchGlobalEvent(eventKey(), EVENT_DEPOT_ASSET_MARKED, path);
    }
}

void AssetBrowserService::markedFiles(Array<StringBuf>& outFiles, int& outTag)
{
    if (m_markedDepotFilesTag != 0)
    {
        outTag = m_markedDepotFilesTag;

        outFiles.reserve(m_markedDepotFiles.size());
        for (const auto& path : m_markedDepotFiles.keys())
            outFiles.pushBack(path);
    }
}

bool AssetBrowserService::checkFileMark(StringView path, int& outTag) const
{
    if (!m_markedDepotFiles.contains(path))
        return false;
    outTag = m_markedDepotFilesTag;
    return true;
}

//--

ResourceEditorPtr AssetBrowserService::findEditor(StringView depotPath) const
{
    ResourceEditorPtr ret;

    auto ed = GetService<EditorService>();
    ed->iterateWindows<IResourceContainerWindow>([depotPath, &ret](IResourceContainerWindow* w)
        {
            w->findEditor(ret, [depotPath](ResourceEditor* editor)
                {
                    return editor->info().metadataDepotPath == depotPath;
                }, nullptr);
        });

    return ret;
}

Array<ResourceEditorPtr> AssetBrowserService::collectEditors(bool modifiedOnly /*= false*/) const
{
    Array<ResourceEditorPtr> ret;

    auto ed = GetService<EditorService>();
    ed->iterateWindows<IResourceContainerWindow>([modifiedOnly, &ret](IResourceContainerWindow* w)
        {
            w->iterateEditors([modifiedOnly, &ret](ResourceEditor* editor)
                {
                    if (!modifiedOnly || editor->modified())
                        ret.pushBack(AddRef(editor));
                }, nullptr);
        });

    return ret;
}

ResourceEditorPtr AssetBrowserService::openEditor(ui::IElement* owner, StringView depotPath, bool focus)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", nullptr);

    // find existing editor
    if (auto existingEditor = findEditor(depotPath))
    {
        if (focus)
            existingEditor->ensureVisible(true);
        return existingEditor;
    }

    // load the resource's metadata
    ResourceInfo info;
    info.metadataDepotPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    info.metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(info.metadataDepotPath);
    if (!info.metadata)
        return false;

    // load resource's content
    info.resourceExtension = info.metadata->loadExtension;
    info.resourceDepotPath = ReplaceExtension(depotPath, info.resourceExtension);
    GetService<DepotService>()->loadFileToResource(info.resourceDepotPath, info.resource, info.metadata->resourceClassType);
    if (!info.resource)
        return false;

    // custom format ?
    info.customFormat = (info.metadata->loadExtension != IResource::FILE_EXTENSION);

    //--

    // open the editor
    Array<SpecificClassType<IResourceEditorOpener>> openers;
    RTTI::GetInstance().enumClasses(openers);

    ResourceEditorPtr editor;
    for (auto openerClass : openers)
        if (auto opener = openerClass->create<IResourceEditorOpener>())
            if (opener->createEditor(owner, info, editor))
                break;

    if (!editor)
        return false;

    // load the general editor config
    const auto configBlock = config().tag("Resources").tag(depotPath);
    editor->configLoad(configBlock);

    // load the name of the resource container tag 
    auto containerTag = editor->containerTag();
    configBlock.read("ContainerTag", containerTag);

    // create the container window
    auto ed = GetService<EditorService>();
    {
        RefPtr<TabbedResourceContainerWindow> containerWindow;
        ed->iterateWindows<TabbedResourceContainerWindow>([&containerWindow, containerTag](TabbedResourceContainerWindow* w)
            {
                if (w->tag() == containerTag)
                    containerWindow = AddRef(w);
            });

        if (!containerWindow)
        {
            containerWindow = RefNew<TabbedResourceContainerWindow>(containerTag);
            ed->attachWindow(containerWindow);
        }

        containerWindow->attachEditor(editor);
        containerWindow->requestShow(focus);
    }    

    // return created editor
    return editor;
}

//--

#if 0
void EditorService::saveWindows() const
{
    for (const auto& window : m_windows)
        if (window->tag())
            window->configSave(m_configRootBlock->tag(window->tag()));
}

void EditorService::saveOpenedFiles() const
{
    // collect opened files
    HashSet<StringBuf> openedFiles;
    iterateEditors([&openedFiles](ResourceEditor* ed)
        {
            openedFiles.insert(ed->info().metadataDepotPath);
        });

    // store list of opened files
    m_configRootBlock->tag("Editor").write("OpenedFiles", openedFiles.keys());

    // store config for each editor
    for (const auto& editor : collectEditors())
    {
        const auto configBlock = m_configRootBlock->tag("Resources").tag(editor->info().metadataDepotPath);

        if (auto* container = editor->findParent<IBaseResourceContainerWindow>())
            configBlock.write("ContainerTag", container->tag());

        editor->configSave(configBlock);
    }
}

//--

bool EditorService::showEditor(StringView depotPath) const
{
  
}

IBaseResourceContainerWindow* EditorService::findResourceContainer(StringView text) const
{
    for (const auto& window : m_windows)
        if (auto rc = rtti_cast<IBaseResourceContainerWindow>(window))
            if (rc->tag() == text)
                return rc;

    return nullptr;
}

IBaseResourceContainerWindow* EditorService::findOrCreateResourceContainer(StringView text)
{
    DEBUG_CHECK_RETURN_EX_V(!text.empty(), "Invalid container tag", nullptr);

    if (auto window = findResourceContainer(text))
        return window;

    DEBUG_CHECK_RETURN_EX_V(text != "Main", "Invalid container tag", nullptr);

    auto window = RefNew<FloatingResourceContainerWindow>(text);
    attachWindow(window);

    return window;
}
bool EditorService::openEditor(ui::IElement* owner, StringView depotPath, bool activate)
{

}

bool EditorService::closeEditor(ui::IElement* owner, StringView depotPath, bool force)
{
    if (auto editor = findEditor(depotPath))
    {
        editor->handleCloseRequest();
        return true;
    }

    return false;
}

bool EditorService::saveEditor(ui::IElement* owner, StringView depotPath, bool force)
{
    if (auto editor = findEditor(depotPath))
    {
        if (editor->modified() || force)
        {
            if (!editor->save())
            {
                ui::PostWindowMessage(editor, ui::MessageType::Error, "FileSave"_id, TempString("Error saving file '{}'", depotPath));
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}
#endif

END_BOOMER_NAMESPACE_EX(ed)
