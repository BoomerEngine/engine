/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#pragma once

#include "sceneEditorStructureGroup.h"

namespace ed
{
    namespace world
    {

        /// world resource root
        class EDITOR_SCENE_MAIN_API ContentWorldRoot : public ContentGroup
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContentWorldRoot, ContentGroup);

        public:
            ContentWorldRoot(const depot::ManagedDirectoryPtr& dirPtr, const depot::ManagedFilePtr& file);
            ~ContentWorldRoot();

            /// get the world resource
            INLINE const scene::WorldPtr& worldPtr() const { return m_world; }

            /// get the file
            INLINE const depot::ManagedFilePtr& fileHandler() const { return m_fileHandler; }

            //---

            /// get data view of the content object at this position
            /// NOTE: allows the data to be edited directly
            virtual ContentElementMask contentType() const override;
            virtual const base::StringBuf& name() const override;
            virtual scene::NodePath path() const override;
            virtual void buildViewContent(ui::ElementPtr& content) override;

        private:
            depot::ManagedFilePtr m_fileHandler;
            scene::WorldPtr m_world;

            static base::edit::DocumentObjectID CreateRootDocumentID(const depot::ManagedFilePtr& file);
        };

    } // mesh
} // ed