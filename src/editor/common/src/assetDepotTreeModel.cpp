/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "assetDepotTreeModel.h"

#include "managedDepot.h"
#include "managedDirectory.h"

#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetDepotTreeDirectoryItem);
RTTI_END_TYPE();

AssetDepotTreeDirectoryItem::AssetDepotTreeDirectoryItem(StringView depotPath, StringView name)
    : m_depotPath(depotPath)
    , m_name(name)
{
    m_label = createChild<ui::TextLabel>(TempString("{}", m_name));
    checkIfSubFoldersExist();
}

AssetDepotTreeDirectoryItem* AssetDepotTreeDirectoryItem::findChild(StringView name, bool autoExpand /*= true*/)
{
    if (autoExpand)
        expand();

    for (const auto& child : children())
        if (auto dir = rtti_cast<AssetDepotTreeDirectoryItem>(child))
            if (dir->name() == name)
                return dir;

    return nullptr;
}

void AssetDepotTreeDirectoryItem::updateLabel()
{
    if (expanded())
        m_label->text(TempString("[img:folder_open] {}", m_name));
    else
        m_label->text(TempString("[img:folder_closed] {}", m_name));
}

void AssetDepotTreeDirectoryItem::updateButtonState()
{
    updateLabel();
    TBaseClass::updateButtonState();
}

StringBuf AssetDepotTreeDirectoryItem::queryTooltipString() const
{
    return m_depotPath;
}

ui::DragDropDataPtr AssetDepotTreeDirectoryItem::queryDragDropData(const input::BaseKeyFlags& keys, const ui::Position& position) const
{
    return nullptr;
}

ui::DragDropHandlerPtr AssetDepotTreeDirectoryItem::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    return nullptr;
}

void AssetDepotTreeDirectoryItem::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{

}

bool AssetDepotTreeDirectoryItem::handleItemContextMenu(const ui::CollectionItems& items, ui::PopupPtr& outPopup) const
{
    return false;
}

bool AssetDepotTreeDirectoryItem::handleItemFilter(const ui::SearchPattern& filter) const
{
    return filter.testString(m_name);
}

bool AssetDepotTreeDirectoryItem::handleItemSort(const ui::ICollectionItem* other, int colIndex) const
{
    if (const auto* otherEx = rtti_cast<AssetDepotTreeDirectoryItem>(other))
        return m_name < otherEx->m_name;
    return TBaseClass::handleItemSort(other, colIndex);
}

void AssetDepotTreeDirectoryItem::checkIfSubFoldersExist()
{
    m_hasChildren = false;
    GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this](StringView name)
        {
            m_hasChildren = true;
        });
}

bool AssetDepotTreeDirectoryItem::handleItemCanExpand() const
{
    return m_hasChildren;
}

void AssetDepotTreeDirectoryItem::handleItemExpand()
{
    GetService<DepotService>()->enumDirectoriesAtPath(m_depotPath, [this](StringView name)
        {
            auto child = RefNew<AssetDepotTreeDirectoryItem>(TempString("{}{}/", m_depotPath, name), name);
            addChild(child);
        });
}

void AssetDepotTreeDirectoryItem::handleItemCollapse()
{
    removeAllChildren();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetDepotTreeEngineRoot);
RTTI_END_TYPE();

AssetDepotTreeEngineRoot::AssetDepotTreeEngineRoot()
    : AssetDepotTreeDirectoryItem("/engine/", "<engine>")
{}

void AssetDepotTreeEngineRoot::updateLabel()
{
    m_label->text(TempString("[img:database] Engine"));
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetDepotTreeProjectRoot);
RTTI_END_TYPE();

AssetDepotTreeProjectRoot::AssetDepotTreeProjectRoot()
    : AssetDepotTreeDirectoryItem("/project/", "<project>")
{}

void AssetDepotTreeProjectRoot::updateLabel()
{
    m_label->text(TempString("[img:database] Project"));
}

//---

END_BOOMER_NAMESPACE_EX(ed)
