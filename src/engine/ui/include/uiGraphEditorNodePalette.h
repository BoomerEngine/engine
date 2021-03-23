/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "uiSimpleTreeModel.h"
#include "uiElement.h"
#include "uiDragDrop.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///----

/// graph block palette entry
struct GraphBlockPaletteEntry
{
    StringBuf caption;
    StringBuf displayText;
    SpecificClassType<graph::Block> blockClass;

    INLINE bool operator==(const GraphBlockPaletteEntry& other) const { return blockClass == other.blockClass && caption == other.caption; }
};

/// simple block palette tree model
class ENGINE_UI_API GraphBlockPaletteTreeModel : public SimpleTreeModel<GraphBlockPaletteEntry>
{
public:
    GraphBlockPaletteTreeModel();

    void addRootClass(SpecificClassType<graph::Block> rootClass);

private:
    virtual bool compare(const GraphBlockPaletteEntry& a, const GraphBlockPaletteEntry& b, int colIndex) const override;
    virtual bool filter(const GraphBlockPaletteEntry& data, const SearchPattern& filter, int colIndex = 0) const override;
    virtual StringBuf displayContent(const GraphBlockPaletteEntry& data, int colIndex = 0) const override;
    virtual DragDropDataPtr queryDragDropData(const BaseKeyFlags& keys, const ModelIndex& item) override;
    virtual ElementPtr tooltip(const GraphBlockPaletteEntry& data) const override;

    HashMap<StringBuf, ModelIndex> m_groups;

    ModelIndex findOrCreateRootGroup(StringView name);
};

///----

// helper dialog that allows to select a type from type list
class ENGINE_UI_API GraphBlockPalette : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphBlockPalette, IElement);

public:
    GraphBlockPalette();

    void setRootClasses(const Array<SpecificClassType<graph::Block>>& rootClasses);
    void setRootClasses(graph::Container* graph);

private:
    TreeViewPtr m_tree;
    SearchBarPtr m_searchBar;
};

///----

END_BOOMER_NAMESPACE_EX(ui)
