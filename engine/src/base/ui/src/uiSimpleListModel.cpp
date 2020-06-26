/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: model #]
***/

#include "build.h"
#include "uiSimpleListModel.h"
#include "uiTextLabel.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SimpleListModel);
    RTTI_END_TYPE();

    SimpleListModel::SimpleListModel()
    {}

    SimpleListModel::~SimpleListModel()
    {}

    uint32_t SimpleListModel::rowCount(const ModelIndex& parent) const
    {
        if (!parent)
            return size();
        return 0;
    }

    bool SimpleListModel::hasChildren(const ModelIndex& parent) const
    {
        if (!parent)
            return true;
        return false;
    }

    bool SimpleListModel::hasIndex(int row, int col, const ModelIndex& parent) const
    {
        return !parent.valid() && col == 0 && row >= 0 && row < (int)size();
    }

    ModelIndex SimpleListModel::parent(const ModelIndex& item) const
    {
        return ModelIndex();
    }

    ModelIndex SimpleListModel::index(int row, int column, const ModelIndex& parent) const
    {
        if (hasIndex(row, column, parent))
            return ui::ModelIndex(this, row, column);
        return ui::ModelIndex();
    }

    bool SimpleListModel::compare(const ModelIndex& first, const ModelIndex& second, int colIndex) const
    {
        if (colIndex == 0)
            return content(first, colIndex) < content(second, colIndex);
        return first < second;
    }

    bool SimpleListModel::filter(const ModelIndex& id, const ui::SearchPattern& filter, int colIndex) const
    {
        return true;
    }

    base::StringBuf SimpleListModel::displayContent(const ModelIndex& id, int colIndex /*= 0*/) const
    {
        return content(id, colIndex);
    }

    //--

} // ui
