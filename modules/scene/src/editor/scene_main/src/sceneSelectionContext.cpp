/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: actions #]
***/

#include "build.h"
#include "sceneSelectionContext.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructureGroup.h"
#include "sceneEditorStructurePrefabRoot.h"
#include "sceneEditorStructureWorldRoot.h"

#include "scene/common/include/sceneLayer.h"
#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneNodeTemplate.h"

namespace ed
{
    namespace world
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(SelectionContext);
        RTTI_END_TYPE();

        SelectionContext::SelectionContext(ContentStructure& content, const base::Array<ContentElementPtr>& selection)
            : m_content(content)
            , m_selection(selection)
        {
            filterSelection();
        }

        SelectionContext::SelectionContext(ContentStructure& content, const ContentElementPtr& selectionOverride)
            : m_content(content)
        {
            if (selectionOverride)
                m_selection.pushBack(selectionOverride);
        }

        void SelectionContext::filterSelection()
        {
            auto unifiedType = ContentElementType::None;
            uint32_t unifiedNodesCount = 0;

            for (auto& node : m_selection)
            {
                if (node->elementType() < unifiedType)
                {
                    unifiedNodesCount = 0;
                    unifiedType = node->elementType();
                }
                else if (node->elementType() == unifiedType)
                {
                    unifiedNodesCount += 1;
                }
            }

            m_unifiedSelectionType = unifiedType;
            m_unifiedSelection.reserve(unifiedNodesCount);
            m_unifiedSelection.reset();

            for (auto& node : m_selection)
                if (node->elementType() == unifiedType)
                    m_unifiedSelection.pushBack(node);

            m_unifiedOrMask = ContentElementMask();
            m_unifiedAndMask = ContentElementMask();

            if (!m_unifiedSelection.empty())
            {
                m_unifiedAndMask = m_unifiedSelection.front()->contentType();

                for (auto& node : m_unifiedSelection)
                {
                    m_unifiedOrMask |= node->contentType();
                    m_unifiedAndMask &= node->contentType();
                }
            }
        }

        SelectionContext::~SelectionContext()
        {}

        //---

    } // world
} // ed