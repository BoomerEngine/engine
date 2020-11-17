/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "uiGraphEditor.h"
#include "uiGraphEditorNodePalette.h"
#include "uiSearchBar.h"
#include "uiTreeView.h"

#include "base/containers/include/stringBuilder.h"
#include "base/graph/include/graphContainer.h"
#include "base/graph/include/graphBlock.h"

namespace ui
{
    //---

    GraphBlockPaletteTreeModel::GraphBlockPaletteTreeModel()
    {}

    ModelIndex GraphBlockPaletteTreeModel::findOrCreateRootGroup(base::StringView name)
    {
        ModelIndex ret;
        if (!m_groups.find(name, ret))
        {
            GraphBlockPaletteEntry entry;
            entry.caption = base::StringBuf(name);
            entry.displayText = base::StringBuf(base::TempString("[b]{}[/b]", name));
            ret = addRootNode(entry);
            m_groups[entry.caption] = ret;
        }

        return ret;
    }

    void GraphBlockPaletteTreeModel::addRootClass(base::SpecificClassType<base::graph::Block> rootClass)
    {
        base::InplaceArray<base::ClassType, 100> blockClasses;
        RTTI::GetInstance().enumClasses(rootClass, blockClasses);

        for (const auto blockClass : blockClasses)
        {
            if (const auto* groupInfo = blockClass->findMetadata<base::graph::BlockInfoMetadata>())
            {
                base::StringView groupName = "Generic";
                if (!groupInfo->groupString.empty())
                    groupName = groupInfo->groupString;

                base::StringView blockName = groupInfo->nameString;
                if (blockName.empty())
                    blockName = groupInfo->titleString;

                if (!blockName.empty())
                {
                    auto groupId = findOrCreateRootGroup(groupInfo->groupString);
                    
                    GraphBlockPaletteEntry entry;
                    entry.caption = base::StringBuf(blockName);
                    entry.blockClass = blockClass.cast<base::graph::Block>();
                    entry.displayText = base::StringBuf(base::TempString("[img:fx] {}", blockName));
                    addChildNode(groupId, entry);
                }
            }
        }
    }

    bool GraphBlockPaletteTreeModel::compare(const GraphBlockPaletteEntry& a, const GraphBlockPaletteEntry& b, int colIndex) const
    {
        return a.caption < b.caption;
    }

    bool GraphBlockPaletteTreeModel::filter(const GraphBlockPaletteEntry& data, const SearchPattern& filter, int colIndex /*= 0*/) const
    {
        return filter.testString(data.caption);
    }

    base::StringBuf GraphBlockPaletteTreeModel::displayContent(const GraphBlockPaletteEntry& data, int colIndex /*= 0*/) const
    {
        return data.displayText;
    }

    DragDropDataPtr GraphBlockPaletteTreeModel::queryDragDropData(const base::input::BaseKeyFlags& keys, const ModelIndex& item)
    {
        if (const auto data = dataPtrForNode(item))
        {
            if (data->blockClass)
                return base::CreateSharedPtr<GraphBlockClassDragDropData>(data->blockClass);
        }

        return nullptr;
    }

    ui::ElementPtr GraphBlockPaletteTreeModel::tooltip(const GraphBlockPaletteEntry& data) const
    {
        if (data.blockClass)
            return CreateGraphBlockTooltip(data.blockClass);
        return nullptr;
    }

    //---

    RTTI_BEGIN_TYPE_CLASS(GraphBlockPalette);
    RTTI_END_TYPE();

    GraphBlockPalette::GraphBlockPalette()
    {
        layoutVertical();

        m_searchBar = createChild<SearchBar>(false);

        m_tree = createChild<TreeView>();
        m_tree->expand();
        m_tree->expandAll();

        m_searchBar->bindItemView(m_tree);
    }

    void GraphBlockPalette::setRootClasses(base::graph::Container* graph)
    {
        base::InplaceArray<base::SpecificClassType<base::graph::Block>, 10> blockClasses;
        if (graph)
            graph->supportedBlockClasses(blockClasses);

        setRootClasses(blockClasses);
    }

    void GraphBlockPalette::setRootClasses(const base::Array<base::SpecificClassType<base::graph::Block>>& rootClasses)
    {
        auto model = base::CreateSharedPtr<GraphBlockPaletteTreeModel>();

        for (const auto blockClass : rootClasses)
            model->addRootClass(blockClass);

        m_tree->model(model);
    }

    //---

} // ui

