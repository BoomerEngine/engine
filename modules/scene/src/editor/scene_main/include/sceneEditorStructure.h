/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#pragma once

#include "base/containers/include/mutableArray.h"
#include "base/containers/include/threadSafeAppendArray.h"

#include "scene/common/include/sceneNodePath.h"
#include "editor/asset_browser/include/managedDepot.h"
#include "editor/asset_browser/include/managedDirectory.h"
#include "editor/asset_browser/include/managedFile.h"

#include "ui/models/include/uiAbstractItemModel.h"
#include "ui/toolkit/include/uiButton.h"
#include "ui/toolkit/include/uiStaticContent.h"

namespace ed
{
    namespace world
    {
        ///--

        class ContentStructureSupplier;
        class ContentStructure;
        class ContentLayer;

        ///---

        /// generalized type of content
        enum class ContentElementBit : uint32_t
        {
            Invalid = 0,

            Root = FLAG(1),
            Group = FLAG(2),
            Layer = FLAG(3),
            Node = FLAG(4),
            NodeContainer = FLAG(5),
            World = FLAG(6),
            FromPrefab = FLAG(7),
            Prefab = FLAG(8),
            HasObject = FLAG(9),
            CanDelete = FLAG(10),
            CanCopy = FLAG(11),
            CanBePasteRoot = FLAG(12),
            LayerContainer = FLAG(13),
            GroupContainer = FLAG(14),
            Modified = FLAG(15),
            Active = FLAG(16),
            CanActivate = FLAG(17),
            Visible = FLAG(18),
            CanToggleVisibility = FLAG(19),
            CanSave = FLAG(20),
        };

        typedef base::DirectFlags<ContentElementBit> ContentElementMask;

        ///---

        /// content element type
        enum class ContentElementType : uint8_t
        {
            Prefab,
            Group,
            World,
            Layer,
            Node,
            NodeFromPrefab,
            None,
        };

        INLINE static bool operator<(ContentElementType a, ContentElementType b) {
            return (int)a < (int)b;
        }

        ///---

        enum class ContentDirtyFlagBit : uint8_t
        {
            Name = FLAG(0),
            Class = FLAG(1),
            Error = FLAG(2),
            Visibility = FLAG(3),
            Modified = FLAG(4),
        };

        typedef base::DirectFlags<ContentDirtyFlagBit> ContentDirtyFlags;

        ///---

        /// base class for all scene content elements
        class EDITOR_SCENE_MAIN_API IContentElement : public base::IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IContentElement, base::IObject);

        public:
            IContentElement(ContentElementType elementType);
            virtual ~IContentElement();
            
            /// get the "class" of the element, allows for some cheaper filtering
            INLINE ContentElementType elementType() const { return m_elementType; }

            /// get shared pointer from this object
            INLINE base::RefPtr<IContentElement> sharedFromThis() const { return base::ptr_cast<IContentElement>(TBaseClass::sharedFromThis()); }

            /// get the content structure we belong to
            INLINE ContentStructure* scene() const { return m_structure; }

            /// get managed children list for UI
            INLINE const base::Array<IContentElement*> children() const { return m_children; }

            /// get the parent element, may be NULL for the root one
            INLINE IContentElement* parent() const { return m_parent; }

            /// get the local visibility flag
            INLINE bool localVisibility() const { return m_localVisibilityFlag; }

            /// get the merged visibility flag
            INLINE bool mergedVisibility() const { return m_mergedVisibilityFlag; }

            //--

            /// toggle node local visibility, may propagate to others
            void toggleLocalVisibility(bool isVisible);

            /// refresh the merged (group + layer) visibility state
            void refreshMergedVisibility();

            //----

            /// resolve a tree like model index
            virtual ui::ModelIndex modelIndex() const;

            /// get name of this content element
            virtual const base::StringBuf& name() const = 0;

            /// get path to represented node
            virtual scene::NodePath path() const;

            /// resolve actual data object related to this element
            /// NOTE: may fail for some of the content elements (like component or group)
            virtual base::ObjectPtr resolveObject(base::ClassType expectedClass) const = 0;

            /// visit the structure
            virtual void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const = 0;

            /// collect modified content elements
            virtual void collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const;

            /// update node visualization, called whenever the merged visibility changes
            virtual void refreshVisualizationVisibility();

            /// get the type of content represented by this node
            virtual ContentElementMask contentType() const;

            /// get type index for sorting
            virtual int compareType() const;

            /// compare for sorting, always compares elements of the same type index
            virtual bool compareForSorting(const IContentElement& other) const;

            //--

            // render the content of this node
            virtual void render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const;

            // create visual representation in the scene
            virtual void attachToScene(const scene::ScenePtr& scene);

            // remove visual representation from the scene
            virtual void detachFromScene(const scene::ScenePtr& scene);

            // attach to scene content structure
            virtual void attachToStructure(ContentStructure* structure);

            // detach from scene content structure
            virtual void detachFromStructure();

            /// attach to content structure
            void attachToParent(IContentElement* parent);

            /// remove from content structure
            void detachFromParent();

            /// unlink all node's children from this node (recursive)
            void detachAllChildren();

            ///--

            /// generate a safe (unique) name for a new object in this container, usually tries to get append numbers at the end until we don't have a name collision
            base::StringBuf generateSafeName(base::StringView coreName) const;

            //--

            // request a content update in the model
            void requestViewUpdate(ContentDirtyFlags flags);

            // build the UI content
            virtual void buildViewContent(ui::ElementPtr& content) = 0;

            // refresh the UI content
            virtual void refreshViewContent(ui::ElementPtr& content);

            //--

            /// resolve actual data object related to this element
            template< typename T >
            INLINE base::RefPtr<T> resolveObject() const
            {
                return base::rtti_cast<T>(resolveObject(T::GetStaticClass()));
            }

            //--

            // add content to this node
            virtual bool addContent(const base::Array<ContentElementPtr>& content);

            // remove content from this node
            virtual bool removeContent(const base::Array<ContentElementPtr>& content);

            // add content to this node
            bool addContent(const ContentElementPtr& content);

            // remove content from this node
            bool removeContent(const ContentElementPtr& content);

        protected:
            IContentElement* m_parent;
            base::Array<IContentElement*> m_children;

            ContentStructure* m_structure;
            ContentElementType m_elementType;
            bool m_localVisibilityFlag:1;
            bool m_mergedVisibilityFlag:1;
            bool m_attchedToScene : 1;
            mutable bool m_visibilityStateChanged:1;

            ContentDirtyFlags m_dirtyFlags;

            /// child node has been added with this node as parent
            virtual void linkChild(IContentElement* child);

            /// child node is being removed, unlink it from loca lists
            virtual void unlinkChild(IContentElement* child);
        };

        ///---

        /// world structure - the classic world / layer groups / layers / nodes
        class EDITOR_SCENE_MAIN_API ContentStructure : public ui::IAbstractItemModel
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ContentStructure);

        public:
            ContentStructure(const depot::ManagedFilePtr& prefabOrWorldFile);
            virtual ~ContentStructure();

            /// get the root resource
            INLINE const depot::ManagedFilePtr& rootFile() const { return m_rootFile; }

            /// get the root interface
            INLINE const ContentElementPtr& root() const { return m_root; }

            /// get source of the content
            INLINE ContentStructureSource source() const { return m_source; }

            /// get the scene we use for visualization of this structure, may be null
            INLINE const scene::ScenePtr& scene() const { return m_scene; }

            /// is this a world file ?
            INLINE bool isWorld() const { return m_source == ContentStructureSource::World; }

            /// is this a prefab file ?
            INLINE bool isPrefab() const { return m_source == ContentStructureSource::Prefab; }

            /// get the active element (for context)
            INLINE const ContentElementPtr& activeElement() const { return m_activeElement; }

            //--

            // visit content structure
            void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func);

            // find node by path
            ContentElementPtr findNode(const scene::NodePath& path) const;

            //--

            // render the content, renders all node templates and all the helpers
            void render(rendering::scene::FrameInfo& frame);

            //--

            // get depot path to given special world directory
            base::StringBuf internalWorldDirectory(const base::StringBuf& name) const;

            //--

            // attach a scene for content visualization
            void attachScene(const scene::ScenePtr& scene);

            // detach a scene used for content visualization
            void deattachScene();

            //--

            struct SaveStats
            {
                uint32_t m_numSavedLayers = 0;
                uint32_t m_numSavedNodes = 0;
                uint32_t m_numTotalNodes = 0;
                uint32_t m_numLayersAdded = 0;
                uint32_t m_numLayersRemoved = 0;
                uint32_t m_numGroupsAdded = 0;
                uint32_t m_numGroupsRemoved = 0;
                base::Array<ContentLayerPtr> m_failedLayers;
            };

            // save all modified content 
            void saveContent(SaveStats& outStats);
            void saveContent(depot::ManagedDirectory* dir, const ContentGroupPtr& group, SaveStats& outStats);

            //--

            // enable selection visualization on objects from the list, has similar effects to just going node by node and setting selection flag
            void visualizeSelection(const base::Array<ContentNodePtr>& selectionList);

            //--

            // activate element (as context)
            void activeElement(const ContentElementPtr& element);

            //--

            // register a node for streaming
            void attachStreamableNode(ContentNode* node);

            /// detach streamable node
            void detachStreamableNode(ContentNode* node);

        public:
            // source of the data
            ContentStructureSource m_source;

            // visualization scene
            scene::ScenePtr m_scene;

            // root file (prefab or world file)
            depot::ManagedFilePtr m_rootFile;

            // root content group
            ContentElementPtr m_root;

            // visualized selection nodes
            base::HashSet<ContentNodePtr> m_visualizedSelection;

            // active context element
            ContentElementPtr m_activeElement;

            //--

            base::HashSet<ContentNode*> m_streamableNodes;

            //--

            virtual uint32_t rowCount(const ui::ModelIndex& parent) const override final;
            virtual bool hasChildren(const ui::ModelIndex& parent) const override final;
            virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent) const override final;
            virtual ui::ModelIndex parent(const ui::ModelIndex& item) const override final;
            virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent) const override final;
            virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
            virtual bool filter(const ui::ModelIndex& id, base::StringView filter, int colIndex = 0) const override final;
            virtual void visualize(const ui::ModelIndex& item, const ui::ItemVisiualizationMode mode, ui::ElementPtr& content) const override final;
            virtual base::RefPtr<ui::PopupMenu> contextMenu(const base::Array<ui::ModelIndex>& indices) const override final;

            //--

            // add/remove node from selection list
            void updateVisualSelectionList(ContentNode& node, bool state);

            friend class ContentNode;
        };

        ///--

    } // mesh
} // ed