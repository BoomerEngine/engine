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

#include "core/containers/include/stringBuilder.h"
#include "core/graph/include/graphContainer.h"
#include "core/graph/include/graphBlock.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

GraphBlockPaletteTreeModel::GraphBlockPaletteTreeModel()
{}

ModelIndex GraphBlockPaletteTreeModel::findOrCreateRootGroup(StringView name)
{
    ModelIndex ret;
    if (!m_groups.find(name, ret))
    {
        GraphBlockPaletteEntry entry;
        entry.caption = StringBuf(name);
        entry.displayText = StringBuf(TempString("[b]{}[/b]", name));
        ret = addRootNode(entry);
        m_groups[entry.caption] = ret;
    }

    return ret;
}

void GraphBlockPaletteTreeModel::addRootClass(SpecificClassType<graph::Block> rootClass)
{
    InplaceArray<ClassType, 100> blockClasses;
    RTTI::GetInstance().enumClasses(rootClass, blockClasses);

    for (const auto blockClass : blockClasses)
    {
        if (const auto* groupInfo = blockClass->findMetadata<graph::BlockInfoMetadata>())
        {
            StringView groupName = "Generic";
            if (!groupInfo->groupString.empty())
                groupName = groupInfo->groupString;

            StringView blockName = groupInfo->nameString;
            if (blockName.empty())
                blockName = groupInfo->titleString;

            if (!blockName.empty())
            {
                auto groupId = findOrCreateRootGroup(groupInfo->groupString);
                    
                GraphBlockPaletteEntry entry;
                entry.caption = StringBuf(blockName);
                entry.blockClass = blockClass.cast<graph::Block>();
                entry.displayText = StringBuf(TempString("[img:fx] {}", blockName));
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

StringBuf GraphBlockPaletteTreeModel::displayContent(const GraphBlockPaletteEntry& data, int colIndex /*= 0*/) const
{
    return data.displayText;
}

DragDropDataPtr GraphBlockPaletteTreeModel::queryDragDropData(const BaseKeyFlags& keys, const ModelIndex& item)
{
    if (const auto data = dataPtrForNode(item))
    {
        if (data->blockClass)
            return RefNew<GraphBlockClassDragDropData>(data->blockClass);
    }

    return nullptr;
}

ElementPtr GraphBlockPaletteTreeModel::tooltip(const GraphBlockPaletteEntry& data) const
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

void GraphBlockPalette::setRootClasses(graph::Container* graph)
{
    InplaceArray<SpecificClassType<graph::Block>, 10> blockClasses;
    if (graph)
        graph->supportedBlockClasses(blockClasses);

    setRootClasses(blockClasses);
}

void GraphBlockPalette::setRootClasses(const Array<SpecificClassType<graph::Block>>& rootClasses)
{
    auto model = RefNew<GraphBlockPaletteTreeModel>();

    for (const auto blockClass : rootClasses)
        model->addRootClass(blockClass);

    m_tree->model(model);
}

//---

END_BOOMER_NAMESPACE_EX(ui)

