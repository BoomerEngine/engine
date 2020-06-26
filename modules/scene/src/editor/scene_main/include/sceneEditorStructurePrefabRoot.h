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

        /// prefab resource root
        class EDITOR_SCENE_MAIN_API ContentPrefabRoot : public IContentElement 
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContentPrefabRoot, IContentElement);

        public:
            ContentPrefabRoot(const depot::ManagedFilePtr& file);
            ~ContentPrefabRoot();

            /// get the prefab
            INLINE const scene::PrefabPtr& prefabResource() const { return m_prefab; }

            /// get the file handler
            INLINE const depot::ManagedFilePtr& fileHandler() const { return m_fileHandler; }

            /// is this prefab modified ?
            INLINE bool isModified() const { return m_isModified; }

            /// get nodes (only in loaded layers)
            typedef base::Array<ContentNodePtr> TNodes;
            INLINE const TNodes& nodes() const { return m_nodes; }

            //--

            /// IContentElement
            virtual base::ObjectPtr resolveObject(base::ClassType expectedClass) const override;
            virtual void visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const override;
            virtual void refreshVisualizationVisibility() override;
            virtual ContentElementMask contentType() const override;
            virtual void render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const override;
            virtual void collectModifiedContent(base::Array<ContentElementPtr>& outModifiedContent) const override;
            virtual const base::StringBuf& name() const override;
            virtual scene::NodePath path() const override;
            virtual void buildViewContent(ui::ElementPtr& content) override;

        private:
            depot::ManagedFilePtr m_fileHandler;
            bool m_isModified;

            scene::PrefabPtr m_prefab;

            TNodes m_nodes;

            //--

            void syncNodes();
            void removeNode(const scene::NodeTemplate* node);
        };

    } // mesh
} // ed