/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureGroup.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructureWorldRoot.h"
#include "sceneEditorStructurePrefabRoot.h"
#include "sceneEditorHelperObjects.h"
#include "sceneSelectionContext.h"
#include "sceneContextPopup.h"

#include "scene/common/include/sceneRuntime.h"
#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneLayer.h"
#include "scene/common/include/scenePrefab.h"

#include "rendering/scene/include/renderingFrame.h"
#include "rendering/scene/include/renderingFrameScreenCanvas.h"

#include "editor/asset_browser/include/managedFileFormat.h"

namespace ed
{
    namespace world
    {

        //---

        base::ConfigProperty<float> cvSceneNodeTemplateRenderingDistance("Scene", "NodeTemplateRenderingDistance", 50.0f);

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IContentElement);
        RTTI_END_TYPE();

        IContentElement::IContentElement(ContentElementType elementType)
            : m_localVisibilityFlag(true)
            , m_mergedVisibilityFlag(true)
            , m_visibilityStateChanged(false)
            , m_attchedToScene(false)
            , m_elementType(elementType)
            , m_parent(nullptr)
            , m_structure(nullptr)
        {
            /*children.SortFunc = [this](base::Array<base::edit::DocumentObjectID>& table)
            {
                base::InplaceArray<const IContentElement*, 100> elements;
                base::InplaceArray<base::edit::DocumentObjectID, 100> missing;

                for (auto& id : table)
                    if (auto ret = m_structure->objectMap().findObject(id).get())
                        elements.pushBack(ret);
                    else
                        missing.pushBack(id);

                std::stable_sort(elements.begin(), elements.end(), [](const IContentElement* a, const IContentElement* b)
                {
                    auto typeA = a ? a->compareType() : -1;
                    auto typeB = b ? b->compareType() : -1;

                    if (typeA == -1 || typeB == -1)
                        return (char*)a < (char*)b;

                    if (typeA != typeB)
                        return typeA < typeB;

                    return a->compareForSorting(*b);
                });

                table.reset();
                for (auto ptr  : elements)
                    table.pushBack(ptr->documentId());
                for (auto& id : missing)
                    table.pushBack(id);
            };*/
        }

        IContentElement::~IContentElement()
        {
            ASSERT_EX(!m_attchedToScene, "Node is stil attached to visualization scene");
            ASSERT_EX(m_structure == nullptr, "Deleted node should not be part of any scene structure");
            ASSERT_EX(m_parent == nullptr, "Deleted node should not be part of any scene graph");
            ASSERT_EX(m_children.empty(), "Deleted node should not contain un-deleted children");
        }

        void IContentElement::attachToStructure(ContentStructure* structure)
        {
            ASSERT_EX(m_structure == nullptr, "Node is already part of scene structure");

            m_structure = structure;

            for (auto child  : m_children)
                child->attachToStructure(structure);

            if (m_structure->scene() && !m_attchedToScene)
            {
                attachToScene(m_structure->scene());
                ASSERT_EX(m_attchedToScene, "Node not marked as attached to scene");
            }
        }

        void IContentElement::detachFromStructure()
        {
            ASSERT_EX(m_structure != nullptr, "Node is not part of scene structure");

            for (auto child  : m_children)
                child->detachFromStructure();

            if (m_structure->activeElement().get() == this)
                m_structure->activeElement(nullptr);

            if (m_structure->scene() && m_attchedToScene)
            {
                detachFromScene(m_structure->scene());
                ASSERT_EX(!m_attchedToScene, "Node not marked as detached from scene");
            }

            m_structure = nullptr;
        }

        void IContentElement::attachToParent(IContentElement* parent)
        {
            ASSERT_EX(m_parent == nullptr, "Node is already part of scene tree");

            if (parent->m_structure)
                attachToStructure(parent->m_structure);

            if (parent)
            {
                parent->linkChild(this);
                ASSERT(m_parent == parent);
            }
        }

        void IContentElement::detachFromParent()
        {
            if (m_parent != nullptr)
            {
                m_parent->unlinkChild(this);
                ASSERT(m_parent == nullptr);
            }

            if (m_structure)
                detachFromStructure();
        }

        scene::NodePath IContentElement::path() const
        {
            ASSERT(m_parent);
            return m_parent->path()[name()];
        }

        ui::ModelIndex IContentElement::modelIndex() const
        {
            if (!m_parent)
                return ui::ModelIndex(m_structure, 0, 0, sharedFromThis().toWeak());

            auto indexInParent = m_parent->m_children.find((IContentElement*)this);
            ASSERT(indexInParent != INDEX_MAX);

            return ui::ModelIndex(m_structure, indexInParent, 0, sharedFromThis().toWeak());
        }

        void IContentElement::linkChild(IContentElement* child)
        {
            ASSERT(child->m_parent == nullptr);
            ASSERT(!m_children.contains(child));

            if (m_structure != nullptr)
                m_structure->beingInsertRows(modelIndex(), m_children.size(), 1);

            m_children.pushBack(child);
            child->m_parent = this;

            if (m_structure != nullptr)
                m_structure->endInsertRows();
        }

        void IContentElement::unlinkChild(IContentElement* child)
        {
            ASSERT(child->m_parent == this);

            auto index = m_children.find(child);
            ASSERT(index != INDEX_NONE);

            if (m_structure != nullptr)
                m_structure->beingRemoveRows(modelIndex(), index, 1);

            m_children.erase(index);

            if (m_structure != nullptr)
                m_structure->endRemoveRows();

            child->m_parent = nullptr;
        }

        int IContentElement::compareType() const
        {
            return -1;
        }

        bool IContentElement::compareForSorting(const IContentElement& other) const
        {
            return (char*)this < (char*)&other;
        }

        void IContentElement::render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const
        {
        }

        void IContentElement::attachToScene(const scene::ScenePtr& scene)
        {
            ASSERT(!m_attchedToScene);

            for (auto child  : m_children)
            {
                if (!child->m_attchedToScene)
                    child->attachToScene(scene);
            }

            m_attchedToScene = true;
        }

        void IContentElement::detachFromScene(const scene::ScenePtr& scene)
        {
            ASSERT(m_attchedToScene);

            for (auto child  : m_children)
            {
                if (child->m_attchedToScene)
                    child->detachFromScene(scene);
            }

            m_attchedToScene = false;
        }

        void IContentElement::detachAllChildren()
        {
            auto children = m_children;
            for (auto& child : m_children)
            {
                child->detachAllChildren();
                child->detachFromParent();
            }

            ASSERT_EX(m_children.empty(), "Not all children were purged");
        }

        ContentElementMask IContentElement::contentType() const
        {
            ContentElementMask ret;

            if (m_structure && m_structure->activeElement().get() == this)
                ret |= ContentElementBit::Active;

            return ret;
        }

        void IContentElement::toggleLocalVisibility(bool isVisible)
        {
            if (m_localVisibilityFlag != isVisible)
            {
                m_localVisibilityFlag = isVisible;
                m_visibilityStateChanged = true;
                refreshMergedVisibility();
            }
        }

        void IContentElement::refreshMergedVisibility()
        {
            auto mergedVisibility = m_parent->mergedVisibility() && m_localVisibilityFlag;
            if (mergedVisibility != m_mergedVisibilityFlag)
            {
                m_mergedVisibilityFlag = mergedVisibility;
                m_visibilityStateChanged = true;
                refreshVisualizationVisibility();
            }
        }

        void IContentElement::refreshVisualizationVisibility()
        {
            // implementation dependent
        }

        void IContentElement::collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const
        {
            for (auto child  : m_children)
                child->collectModifiedContent(outModifiedContent);
        }

        void IContentElement::requestViewUpdate(ContentDirtyFlags flags)
        {
            if (m_structure)
            {
                bool wasEmpty = m_dirtyFlags.empty();
                m_dirtyFlags |= flags;

                if (wasEmpty)
                    m_structure->requestItemUpdate(modelIndex());
            }
        }

        void IContentElement::refreshViewContent(ui::ElementPtr& content)
        {
            if (m_dirtyFlags.test(ContentDirtyFlagBit::Name))
            {
                if (auto caption = content->findChildByName<ui::StaticContent>("NodeName"))
                    caption->text(name());

                m_dirtyFlags -= ContentDirtyFlagBit::Name;
            }
        }        

        base::StringBuf IContentElement::generateSafeName(base::StringView<char> coreName) const
        {
            base::HashSet<base::StringView<char>> activeNames; // NOTE: make sure we don't put temp strings here!

            activeNames.reserve(m_children.size());
            for (auto child  : m_children)
                activeNames.insert(child->name().view());

            auto baseName = coreName.trimTailNumbers();

            base::StringBuf testName(coreName);
            uint32_t index = 0;
            for (;;)
            {
                if (!activeNames.contains(testName))
                    return testName;

                // create new name
                testName = base::TempString("{}{}", baseName, index);
                index += 1;
            }
        }

        bool IContentElement::addContent(const ContentElementPtr& content)
        {
            if (!content)
                return true;

            base::InplaceArray<ContentElementPtr, 1> ar;
            ar.pushBack(content);

            return addContent(ar);
        }

        bool IContentElement::removeContent(const ContentElementPtr& content)
        {
            if (!content)
                return true;

            base::InplaceArray<ContentElementPtr, 1> ar;
            ar.pushBack(content);

            return removeContent(ar);
        }

        bool IContentElement::addContent(const base::Array<ContentElementPtr>& content)
        {
            return false;
        }

        bool IContentElement::removeContent(const base::Array<ContentElementPtr>& content)
        {
            return false;
        }

#if 0

        static auto ICON_VISIBLE_LOCAL = "eye"_id;
        static auto ICON_VISIBLE_DISABLED = "eye_gray"_id;
        static auto ICON_HIDDEN = "eye_cross_gray"_id;

        static base::StringID GetVisibilityIconName(const IContentElement& elem)
        {
            auto localVisibility = elem.localVisibility();
            auto mergedVisibility = elem.mergedVisibility();

            if (mergedVisibility)
                return localVisibility ? ICON_VISIBLE_LOCAL : ICON_HIDDEN;
            else
                return localVisibility ? ICON_VISIBLE_DISABLED : ICON_HIDDEN;
        }

        void IContentElement::refreshUIContent(uint32_t column, ui::IElement* owner, ui::ElementPtr& content) const
        {
            if (column == COL_SHOW_HIDE)
            {
                if (content)
                {
                    // update visibility icon
                    if (m_visibilityStateChanged)
                    {
                        if (auto icon = content->findChildByName<ui::StaticContent>("VisibilityIcon"))
                            icon->customImage(GetVisibilityIconName(*this));
                        m_visibilityStateChanged = false;
                    }
                }
                else
                {
                    // create the button, make it single click button (Fast response)
                    auto visibilityButton = base::CreateSharedPtr<ui::Button>();
                    visibilityButton->fastMode(true);

                    // create the icon
                    auto visibilityIcon = base::CreateSharedPtr<ui::StaticContent>();
                    visibilityIcon->customImage(GetVisibilityIconName(*this));
                    visibilityIcon->name("VisibilityIcon");
                    visibilityButton->attachChild(visibilityIcon);

                    // connect to visibility change
                    auto weakSelfRef = sharedFromThisType<IContentElement>().toWeak();
                    visibilityButton->OnClick = [weakSelfRef](UI_CALLBACK)
                    {
                        if (auto element = weakSelfRef.lock())
                            element->toggleLocalVisibility(!element->localVisibility());
                    };

                    content = visibilityButton;
                    owner->attachChild(content);
                }
            }
        }
#endif

        //---

        RTTI_BEGIN_TYPE_CLASS(ContentStructure);
        RTTI_END_TYPE();

        //---

        ContentStructure::ContentStructure(const depot::ManagedFilePtr& filePtr)
            : m_rootFile(filePtr)
            , m_root(nullptr)
        {
            // world ?
            if (filePtr->fileFormat().extension() == "v4world")
            {
                // get the layers directory for the world
                auto worldFileDirectory = filePtr->parentDirectory().lock();
                auto worldLayersDirectory = worldFileDirectory->addChildDirectory("layers");
                if (!worldLayersDirectory)
                {
                    TRACE_ERROR("Scene: Unable to get the layers directory from directory '%hs'", worldFileDirectory->depotPath().c_str());
                }

                m_source = ContentStructureSource::World;

                auto root  = base::CreateSharedPtr<ContentWorldRoot>(worldLayersDirectory, filePtr);
                root->attachToStructure(this);
                root->syncInitialStructure(worldLayersDirectory);
                m_root = root;
            }
            else
            {
                m_source = ContentStructureSource::Prefab;

                auto root = base::CreateSharedPtr<ContentPrefabRoot>(filePtr);
                root->attachToStructure(this);
                m_root = root;
            }

        }

        ContentStructure::~ContentStructure()
        {
            ASSERT_EX(m_scene.get() == nullptr, "Scene should be detached by the time the content structure is destroyed");

            // destroy scene structure
            m_root->detachFromStructure();
            m_root->detachAllChildren();
            m_root.reset();
        }

        ContentElementPtr ContentStructure::findNode(const scene::NodePath& path) const
        {
            if (path.empty())
                return m_root;

            auto parent = findNode(path.parent());
            if (!parent)
                return nullptr;

            auto name = path.lastName();
            for (auto child  : parent->children())
            {
                if (child->name() == name.view())
                    return child->sharedFromThis();
            }

            return nullptr;
        }

        void ContentStructure::activeElement(const ContentElementPtr& element)
        {
            auto oldActiveElement = m_activeElement;

            m_activeElement = element;

            if (oldActiveElement)
                oldActiveElement->requestViewUpdate(ContentDirtyFlagBit::Name);

            if (m_activeElement)
                m_activeElement->requestViewUpdate(ContentDirtyFlagBit::Name);
        }

        void ContentStructure::attachScene(const scene::ScenePtr& scene)
        {
            ASSERT_EX(m_scene == nullptr, "Scene is already attached");

            // set scene
            m_scene = scene;

            // bind the scene parameters
            // TODO: this is a hack
            if (auto worldFile = m_rootFile->loadEditableContent<scene::World>())
                scene->attachWorldContent(worldFile);

            // attach all content modes
            m_root->attachToScene(m_scene);
        }

        void ContentStructure::deattachScene()
        {
            ASSERT_EX(m_scene != nullptr, "Scene is not attached");

            // detach all content nodes
            m_root->detachFromScene(m_scene);

            // remove the content supplier from runtime scene
            if (auto worldFile = m_rootFile->loadEditableContent<scene::World>())
                m_scene->detachWorldContent(worldFile);

            // remove scene
            m_scene.reset();
        }

        void ContentStructure::visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func)
        {
            m_root->visitStructure(func);
        }

        void ContentStructure::render(rendering::scene::FrameInfo& frame)
        {
            auto sqDistance = cvSceneNodeTemplateRenderingDistance.get() * cvSceneNodeTemplateRenderingDistance.get();
            m_root->render(frame, sqDistance);
        }

        base::StringBuf ContentStructure::internalWorldDirectory(const base::StringBuf& name) const
        {
            ASSERT(!name.empty());

            if (isWorld())
            {
                auto worldPath = rootFile()->depotPath();
                return base::TempString("{}/{}/", worldPath.stringBeforeLast("/"), name);
            }

            return base::StringBuf();
        }

        /*void ContentStructure::onDocumentSelectionChanged(base::edit::DocumentHandler* doc, const base::edit::DocumentObjectIDSet& currentSelection, const base::edit::DocumentObjectIDSet& newSelection)
        {
            for (auto& id : currentSelection.ids())
            {
                if (!newSelection.contains(id))
                {
                    if (auto node = base::rtti_cast<ContentNode>(m_objectMap.findObject(id)))
                        node->localSelectionFlag(false);
                }
            }

            for (auto& id : newSelection.ids())
            {
                if (!currentSelection.contains(id))
                {
                    if (auto node = base::rtti_cast<ContentNode>(m_objectMap.findObject(id)))
                        node->localSelectionFlag(true);
                }
            }
        }*/

        //---

        void ContentStructure::saveContent(SaveStats& outStats)
        {
            if (m_source == ContentStructureSource::World)
            {
                if (auto rootGroup = base::rtti_cast<ContentGroup>(m_root))
                {
                    if (auto worldLayersDirectory = m_rootFile->parentDirectoryPtr()->addChildDirectory("layers"))
                    {
                        saveContent(worldLayersDirectory.get(), rootGroup, outStats);
                    }
                }
            }
        }

        static depot::ManagedFilePtr GetOrCreateFile(depot::ManagedDirectory* dir, const base::StringBuf& filePath, bool& needsSave, uint32_t& numAdded)
        {
            auto file = dir->depot()->findManagedFile(filePath);
            if (file)
            {
                if (file->isDeleted())
                {
                    TRACE_INFO("Layer file '{}' was previously deleted and will be restored", filePath);
                    file->unmarkAsDeleted();
                    needsSave = true;
                }

                return file;
            }

            TRACE_INFO("Layer file '{}' will be created", filePath);

            auto fileName = filePath.stringAfterLast("/");
            needsSave = true;
            numAdded += 1;
            return dir->addFile(fileName);
        }

        static depot::ManagedDirectoryPtr GetOrCreateDirectory(depot::ManagedDirectory* dir, const base::StringBuf& dirName, uint32_t& numAdded)
        {
            auto childDir = dir->childDirectory(dirName);
            if (childDir)
            {
                if (childDir->isDeleted())
                {
                    TRACE_INFO("Layer group '{}' was previously deleted and will be restored", childDir->depotPath());
                    childDir->unmarkAsDeleted();
                }

                return childDir;
            }

            TRACE_INFO("Layer group '{}{}/' will be created", dir->depotPath(), dirName);
            return dir->addChildDirectory(dirName);
        }

        void ContentStructure::saveContent(depot::ManagedDirectory* dir, const ContentGroupPtr& group, SaveStats& outStats)
        {
            auto depot  = m_rootFile->depot();

            // create/save layer files
            base::Array<depot::ManagedFilePtr> usedLayerFiles;
            for (auto& layer : group->layers())
            {
                outStats.m_numTotalNodes += layer->nodes().size();

                // should we write the file ?
                bool needsSave = layer->isModified();
                if (auto file = GetOrCreateFile(dir, layer->filePath(), needsSave, outStats.m_numLayersAdded))
                {
                    // keep track of used files
                    usedLayerFiles.pushBack(file);

                    // save
                    if (needsSave)
                    {
                        outStats.m_numSavedLayers += 1;

                        // create new layer object
                        if (auto layerData = file->loadEditableContent<scene::Layer>())
                        {
                            // save into data container
                            auto layerNodeContainer = base::CreateSharedPtr<scene::NodeTemplateContainer>();
                            if (layer->saveIntoNodeContainer(layerNodeContainer))
                            {
                                // store data
                                outStats.m_numSavedNodes += layerNodeContainer->nodes().size();
                                layerData->content(layerNodeContainer);

                                // write content
                                if (file->saveSpecificEditableContent(layerData))
                                {
                                    layer->unmarkModified();
                                }
                                else
                                {
                                    TRACE_ERROR("Layer '{}' failed to save modified content", layer->filePath());
                                    outStats.m_failedLayers.pushBack(layer);
                                }
                            }
                            else
                            {
                                TRACE_ERROR("Layer '{}' failed to generate modified content", layer->filePath());
                                outStats.m_failedLayers.pushBack(layer);
                            }
                        }
                    }
                }
            }

            // delete layer files no longer used
            if (dir->files().size() != usedLayerFiles.size())
            {
                auto existingFiles = dir->files();
                for (auto& childFile : existingFiles)
                {
                    // only layer files can be deleted
                    if (!childFile->name().endsWith("v4layer"))
                        continue;;

                    // skip files that are already deleted
                    if (childFile->isDeleted())
                        continue;

                    // skip files that we actually use
                    if (usedLayerFiles.contains(childFile))
                        continue;

                    // file is no longer needed
                    TRACE_INFO("Layer file '{}' is not longer used and will be removed", childFile->depotPath());
                    dir->removeFile(childFile);
                    outStats.m_numLayersRemoved += 1;
                }
            }

            // create missing directories
            base::Array<depot::ManagedDirectoryPtr> usedDirectories;
            for (auto& childGroup : group->groups())
            {
                if (auto childDir = GetOrCreateDirectory(dir, childGroup->name(), outStats.m_numGroupsAdded))
                {
                    // keep track of used groups
                    usedDirectories.pushBack(childDir);

                    // update child groups
                    saveContent(childDir.get(), childGroup, outStats);
                }
            }

            /*// delete groups no longer used
            for (auto& childGroup : dir->directories())
            {
                // skip dirs that are already deleted
                if (childGroup->isDeleted())
                    continue;

                // skip files that we actually use
                if (usedDirectories.contains(childGroup))
                    continue;

                // file is no longer needed
                TRACE_INFO("Layer group '{}' is not longer used and will be removed", childGroup->depotPath());
                dir->removeChildDirectory(childGroup);
                outStats.m_numGroupsRemoved += 1;
            } */
        }

        //---

        void ContentStructure::visualizeSelection(const base::Array<ContentNodePtr>& selectionList)
        {
            base::HashSet<ContentNodePtr> incoming;
            incoming.reserve(selectionList.size());

            for (auto& ptr : selectionList)
            {
                incoming.insert(ptr);

                if (!m_visualizedSelection.contains(ptr))
                    ptr->visualSelectionFlag(true, false);
            }

            for (auto& ptr : m_visualizedSelection.keys())
                if (!incoming.contains(ptr))
                    ptr->visualSelectionFlag(false, false);

            m_visualizedSelection = incoming;
        }

        void ContentStructure::updateVisualSelectionList(ContentNode& elem, bool state)
        {
            if (state)
                m_visualizedSelection.insert(elem.sharedFromThisType<ContentNode>());
            else
                m_visualizedSelection.remove(elem.sharedFromThisType<ContentNode>());
        }

        //---

        uint32_t ContentStructure::rowCount(const ui::ModelIndex& parent) const
        {
            if (!parent)
                return 1;

            auto item = parent.unsafe<IContentElement>();
            if (!item)
                return 0;

            return item->children().size();
        }

        bool ContentStructure::hasChildren(const ui::ModelIndex& parent) const
        {
            if (!parent)
                return true;

            auto item = parent.unsafe<IContentElement>();
            if (!item)
                return false;

            return !item->children().empty();
        }

        bool ContentStructure::hasIndex(int row, int col, const ui::ModelIndex& parent) const
        {
            if (!parent)
                return (row == 0) && (col == 0);

            auto item = parent.unsafe<IContentElement>();
            return item && col == 0 && row >= 0 && row <= item->children().lastValidIndex();
        }

        ui::ModelIndex ContentStructure::parent(const ui::ModelIndex& item) const
        {
            if (!item)
                return ui::ModelIndex();

            auto data = item.unsafe<IContentElement>();
            if (!data)
                return ui::ModelIndex();

            auto itemParent = data->parent();
            if (!itemParent)
                return ui::ModelIndex();

            return itemParent->modelIndex();
        }

        ui::ModelIndex ContentStructure::index(int row, int column, const ui::ModelIndex& parent) const
        {
            if (!parent)
            {
                if (row == 0 && column == 0)
                    return m_root->modelIndex();
                else
                    return ui::ModelIndex();
            }

            auto item = parent.unsafe<IContentElement>();
            if (!item)
                return ui::ModelIndex();

            if (column != 0)
                return ui::ModelIndex();

            if (row < 0 || row > item->children().lastValidIndex())
                return ui::ModelIndex();

            return item->children()[row]->modelIndex();
        }

        bool ContentStructure::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex) const
        {
            auto itemFirst = first.unsafe<IContentElement>();
            auto itemSecond = second.unsafe<IContentElement>();

            if (itemFirst->elementType() != itemSecond->elementType())
                return itemFirst->elementType() < itemSecond->elementType();

            return itemFirst->name() < itemSecond->name();
        }

        bool ContentStructure::filter(const ui::ModelIndex& id, base::StringView<char> filter, int colIndex) const
        {
            if (auto item = id.unsafe<IContentElement>())
                return item->name().view().matchStringOrPatter(filter);

            return true;
        }

        void ContentStructure::visualize(const ui::ModelIndex& id, const ui::ItemVisiualizationMode mode, ui::ElementPtr& content) const
        {
            if (auto item = id.unsafe<IContentElement>())
            {
                if (content)
                    item->refreshViewContent(content);
                else
                    item->buildViewContent(content);
            }
        }

        base::RefPtr<ui::PopupMenu> ContentStructure::contextMenu(const base::Array<ui::ModelIndex>& indices) const
        {
            base::Array<ContentElementPtr> elements;
            elements.reserve(indices.size());

            for (auto id : indices)
            {
                elements.pushBack(id.unsafe<IContentElement>()->sharedFromThis());
            }

            auto context = base::CreateSharedPtr<SelectionContext>(const_cast<ContentStructure&>(*this), elements);
            return BuildPopupMenu(context);
        }

        //---

    } // world
} // ed