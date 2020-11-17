/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#pragma once

#include "sceneEditorStructure.h"
#include "scene/common/include/sceneNodeTemplate.h"

namespace ed
{
    namespace world
    {
        ///--

        /// node in the layer, child of Layer, contains other Nodes
        class EDITOR_SCENE_MAIN_API ContentNode : public IContentElement
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContentNode, IContentElement);

        public:
            ContentNode(const scene::NodeTemplatePtr& nodeTemplate);
            ~ContentNode();

            /// get the node template
            INLINE const scene::NodeTemplatePtr& nodeTemplate() const { return m_nodeTemplate; }

            /// get child nodes
            typedef base::Array<ContentNodePtr> TNodes;
            INLINE const TNodes& childNodes() const { return m_childNodes; }

            /// get cached transform
            INLINE const base::AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

            ///--

            /// apply new transform to all related content
            void applyTransform();

            /// refresh visualization of the node
            void refreshVisualization();

            /// destroy visualization of the node
            void destroyVisualization();

            /// evaluate bounding box of the object
            /// NOTE: always contains the placement point but sometimes gets small if the entity is not yet created
            base::Box calcWorldSpaceBoundingBox() const;

            /// set absolute transform to node
            void absoluteTransform(const base::AbsoluteTransform& transform);

            /// toggle selection visualization effect
            void visualSelectionFlag(bool flag, bool updateStructureList=true);

            // determine node name
            void generateUniqueNodeName(const base::HashSet<base::StringID>& existingNames);

            ///---

            /// extract resources that are somehow directly used in this node
            void extractUsedResource(base::HashSet<base::res::ResourcePath>& outResourcePaths) const;

            /// remove node template from structure
            bool removeNode(const scene::NodeTemplate* node);

            // collect node and children
            void collectHierarchy(base::HashSet<ContentNode*>& nodes);

            /// is this an prefab instanced node ?
            bool isInstancedFromPrefab() const;

            ///--

            virtual void markModified() override;
            virtual void attachToScene(const scene::ScenePtr& scene) override;
            virtual void detachFromScene(const scene::ScenePtr& scene) override;
            virtual void attachToStructure(ContentStructure* structure) override;
            virtual void detachFromStructure() override;
            virtual scene::NodePath path() const override;
            virtual base::ObjectPtr resolveObject(base::ClassType expectedClass) const override;
            virtual void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const override;
            virtual void refreshVisualizationVisibility() override;
            virtual ContentElementMask contentType() const override;
            virtual int compareType() const override;
            virtual bool compareForSorting(const IContentElement& other) const override;
            virtual void render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const override;
            virtual const base::StringBuf& name() const override;
            virtual void buildViewContent(ui::ElementPtr& content) override;
            virtual void refreshViewContent(ui::ElementPtr& content) override;

            //--

        protected:
            // children nodes
            TNodes m_childNodes;

            void refreshVisualizationSelectionFlag();

            virtual bool calcVisualizationSelectionFlag() const;

        private:
            // computed absolute transform
            base::AbsoluteTransform m_absoluteTransform;

            // template for the node, always valid, this is the one we edit
            scene::NodeTemplatePtr m_nodeTemplate;
            //base::evt::EventListener m_nodeTemplateObserver;

            // compiled template
            scene::NodeTemplatePtr m_compiledNodeTemplate;

            //--

            // node created from the template
            bool m_localVisualizationSelectionFlag;
            bool m_visualizationSelectionFlag;

            //--

            base::StringBuf captionText() const;
            base::StringBuf captionStyle() const;

            void propertyChanged(base::StringView path);

            void syncNodes();
            void syncTransfromFromNode();
            void syncPropertiesFromNode();
        };

    } // world
} // ed