/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: actions #]
***/

#pragma once

#include "sceneEditorStructure.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureGroup.h"
#include "sceneEditorStructureWorldRoot.h"
#include "sceneEditorStructurePrefabRoot.h"

namespace ed
{
    namespace world
    {
        /// context for selection
        class SelectionContext : public base::SharedFromThis<SelectionContext>
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(SelectionContext);

        public:
            SelectionContext(ContentStructure& content, const base::Array<ContentElementPtr>& selection);
            SelectionContext(ContentStructure& content, const ContentElementPtr& selectionOverride);
            ~SelectionContext();

            //--

            // is the selection empty
            INLINE bool empty() const { return m_selection.empty(); }

            // is the a single object selection
            INLINE bool single() const { return m_selection.size() == 1; }

            // get number of objects in the selection
            INLINE uint32_t size() const { return m_selection.size(); }

            // get content we operate on
            INLINE ContentStructure& content() const { return m_content; }

            // all selected objects
            INLINE const base::Array<ContentElementPtr>& allSelected() const { return m_selection; }

            // get unique selection
            INLINE ContentElementType unifiedSelectionType() const { return m_unifiedSelectionType; }

            // get list of unified elements
            INLINE const base::Array<ContentElementPtr>& unifiedSelection() const { return m_unifiedSelection; }

            // get typed list of unified elements
            template< typename T >
            INLINE const base::Array<T>& unifiedSelection() const { return (const base::Array<base::RefPtr<T>>&) m_selection; }

            // get "or" mask (at least one node has it) of all "features" of selection - unified selection only 
            INLINE ContentElementMask unifiedOrMask() const { return m_unifiedOrMask; }

            // get "and" mask (all nodes have it) of all "features" of selection - unified selection only 
            INLINE const ContentElementMask& unifiedAndMask() const { return m_unifiedAndMask; }

        private:
            // content
            ContentStructure& m_content;

            // all selected objects
            base::Array<ContentElementPtr> m_selection;

            // unique selection - best 
            ContentElementType m_unifiedSelectionType; // type of the unique selection
            base::Array<ContentElementPtr> m_unifiedSelection; // part of selection that has unified type
            ContentElementMask m_unifiedOrMask;
            ContentElementMask m_unifiedAndMask;

            //--

            void filterSelection();
        };

    } // world
} // ed