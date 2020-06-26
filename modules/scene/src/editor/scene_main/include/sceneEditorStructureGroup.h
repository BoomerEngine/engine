/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#pragma once

#include "sceneEditorStructure.h"

namespace ed
{
    namespace world
    {

        ///--

        /// layer group in the world, contains layers and sub groups
        class EDITOR_SCENE_MAIN_API ContentGroup : public IContentElement
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContentGroup, IContentElement);

        public:
            ContentGroup(const base::StringBuf& directoryPath, const base::StringBuf& name);
            virtual ~ContentGroup();

            /// get the parent group
            INLINE ContentGroup* parent() const { return static_cast<ContentGroup*>(IContentElement::parent()); }

            /// get the managed directory used to represent this layer group
            INLINE const base::StringBuf& directoryPath() const { return m_directoryPath; }

            /// get world path to this group (typically something like "world/city/downtown/hospital/interior"
            /// NOTE: the paths are always relative to the world group
            virtual scene::NodePath path() const override { return m_path; }

            /// get layers
            typedef base::Array<ContentLayerPtr> TLayers;
            INLINE const TLayers& layers() const { return m_layers; }

            /// get sub groups
            typedef base::Array<ContentGroupPtr> TGroups;
            INLINE const TGroups& groups() const { return m_groups; }

            ///---

            /// find child group by name
            ContentGroupPtr findGroup(base::StringID name) const;

            /// find layer by name
            ContentLayerPtr findLayer(base::StringID name) const;

            /// create child group, returns existing one if already exists
            ContentGroupPtr createGroup(base::StringID name);

            /// create layer, returns existing one if already exists
            ContentLayerPtr createLayer(base::StringID name);

            /// remove child group
            bool removeGroup(const ContentGroupPtr& childGroup, bool force = false);

            /// remove child layer
            bool removeLayer(const ContentLayerPtr& childLayer, bool force = false);

            ///---

            /// sync content structure (groups/layers) with actual managed directory
            void syncInitialStructure(const depot::ManagedDirectoryPtr& existingDirectory);

            ///---

            /// collect all dirty layer to save
            void collectLayersToSave(bool recrusive, base::Array<ContentLayer*>& outLayers) const;

            /// check if the group is modified
            bool hasModifiedContent() const;

            //--

            // IContentElement
            virtual base::ObjectPtr resolveObject(base::ClassType expectedClass) const override;
            virtual void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const override;
            virtual void refreshVisualizationVisibility() override;
            virtual ContentElementMask contentType() const override;
            virtual int compareType() const override;
            virtual bool compareForSorting(const IContentElement& other) const override;
            virtual void render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const override;
            virtual const base::StringBuf& name() const override;
            virtual void buildViewContent(ui::ElementPtr& content) override;

        private:
            scene::NodePath m_path;

            base::StringBuf m_directoryPath;
            base::StringBuf m_name;

            TLayers m_layers;
            TGroups m_groups;
        };


    } // mesh
} // ed