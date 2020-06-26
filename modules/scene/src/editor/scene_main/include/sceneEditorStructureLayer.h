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

        /// layer in the world, child of Group, contains Nodes
        /// NOTE: this is just a managed file with "few extra steps"
        /// NOTE: the file abstraction should be sealed from the world structure abstraction
        class EDITOR_SCENE_MAIN_API ContentLayer : public IContentElement
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContentLayer, IContentElement);

        public:
            ContentLayer(const base::StringBuf& layerPath);
            ~ContentLayer();

            /// get path to the layer file 
            INLINE const base::StringBuf& filePath() const { return m_filePath; }

            /// get the parent group
            INLINE ContentGroup* parent() const { return (ContentGroup*)(IContentElement::parent()); }

            /// is this layer modified ?
            INLINE bool isModified() const { return m_modified; }

            /// get nodes (only in loaded layers)
            typedef base::Array<ContentNodePtr> TNodes;
            INLINE const TNodes& nodes() const { return m_nodes; }

            //--

            /// mark layer as modified
            virtual void markModified() override;

            /// mark layer as not modified
            void unmarkModified();

            /// pack layer content into a layer, considered a save
            /// NOTE: this also packs the child nodes and everything else
            bool saveIntoNodeContainer(const scene::NodeTemplateContainerPtr& nodeContainer);

            /// sync initial layer content from a file
            void syncNodes(const depot::ManagedFilePtr& filePtr);

            //--

            // IContentElement
            virtual const base::StringBuf& name() const override;
            virtual ui::ModelIndex modelIndex() const override;
            virtual base::ObjectPtr resolveObject(base::ClassType expectedClass) const override;
            virtual void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const override;
            virtual void refreshVisualizationVisibility() override;
            virtual ContentElementMask contentType() const override;
            virtual int compareType() const override;
            virtual bool compareForSorting(const IContentElement& other) const override;
            virtual void render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const override;
            virtual void collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const override;
            virtual void buildViewContent(ui::ElementPtr& content) override;
            virtual void refreshViewContent(ui::ElementPtr& content) override;
            virtual bool addContent(const base::Array<ContentElementPtr>& content) override;
            virtual bool removeContent(const base::Array<ContentElementPtr>& content) override;

        private:
            base::StringBuf m_name;
            base::StringBuf m_filePath;

            TNodes m_nodes;
            bool m_modified;

            base::image::ImagePtr layerIcon() const;
            base::StringBuf captionText() const;
            base::StringBuf captionStyle() const;

            //--

            void removeNode(const scene::NodeTemplate* node);
        };

    } // mesh
} // ed