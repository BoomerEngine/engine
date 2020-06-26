/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#pragma once

namespace ed
{
    namespace world
    {
        ///---

        class SceneDocumentHandler;

        ///---

        /// container for helper objects, used by nodes
        class EDITOR_SCENE_MAIN_API IHelperObject : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IHelperObject);

        public:
            IHelperObject(const base::RefPtr<ContentNode>& owner);
            virtual ~IHelperObject();

            //---

            /// get owning object ID
            INLINE const base::edit::DocumentObjectID& ownerId() const { return m_ownerId; }

            /// get the owning node
            INLINE const base::RefPtr<ContentNode> owner() const { return m_owner.lock(); }

            /// did the helper object expired ?
            INLINE bool expired() const { return m_owner.expired(); }

            //---

            /// validate, can be used to update the internals of helper object in case the base object changed, returning false will recreate the helper object
            virtual bool validate();

            /// get position of the helper object
            virtual bool placement(const base::edit::DocumentObjectID& id, base::AbsoluteTransform& outTransform);

            /// set placement of the helper object
            virtual bool placement(const base::edit::DocumentObjectID& id, const base::AbsoluteTransform& newTransform);

            /// update selection state
            virtual void handleSelectionChange(const base::edit::DocumentObjectIDSet& oldSelection, const base::edit::DocumentObjectIDSet& newSelection);

            /// handle rendering
            virtual void handleRendering(const base::edit::DocumentObjectIDSet& currentSelection, rendering::scene::FrameInfo& frame);

            //---

        private:
            base::StringID m_category;
            base::edit::DocumentObjectID m_ownerId;
            base::RefWeakPtr<ContentNode> m_owner;
        };

        ///---

        /// handler for helper objects, creates them
        class EDITOR_SCENE_MAIN_API IHelperObjectHandler : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IHelperObjectHandler);

        public:
            IHelperObjectHandler();
            virtual ~IHelperObjectHandler();

            //--

            // reset state (done on document change)
            void reset();

            // set active selection, will use active categories to create helper objects
            void activeSelection(ContentStructure& tree, const base::edit::DocumentObjectIDSet& selection);

            // render helper objects
            void render(rendering::scene::FrameInfo& frame);

            //--

            /// do we support given object category ?
            virtual bool supportsObjectCategory(base::StringID category) const = 0;

            /// create helper object for given node, needs to be overridden
            virtual const base::RefPtr<IHelperObject> createHelperObject(const base::RefPtr<ContentNode>& node) const = 0;

            /// get existing helper object for given ID
            const base::RefPtr<IHelperObject> findHelperObject(const base::edit::DocumentObjectID& id) const;

        private:
            struct Node
            {
                base::edit::DocumentObjectID m_id; // object ID
                base::RefWeakPtr<ContentNode> m_node;
                base::edit::DocumentObjectIDSet m_selection; // local to the object
                base::RefPtr<IHelperObject> m_helper;
                bool m_helperCreated = false;
            };

            base::HashMap<base::edit::DocumentObjectID, Node*> m_nodeMap;
            base::Array<Node*> m_activeNodes;

            base::edit::DocumentObjectIDSet m_activeSelection; // all objects

            void removeExpiredHelperObjects();
        };

        ///---

    } // world
} // ed