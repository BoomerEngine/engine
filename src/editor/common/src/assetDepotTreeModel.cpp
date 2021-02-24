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

BEGIN_BOOMER_NAMESPACE(ed)

//---

AssetDepotTreeModel::AssetDepotTreeModel(ManagedDepot* depot)
    : m_depot(depot)
    , m_depotEvents(this)
{
    if (auto rootDirectory = depot->root())
        addDirNode(ui::ModelIndex(), rootDirectory);

    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_CREATED) = [this](ManagedDirectoryPtr dir)
    {
        if (auto parentDir = dir->parentDirectory())
            if (auto parentEntry = findNodeForData(parentDir))
                addChildNode(parentEntry, dir);
    };

    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_DELETED) = [this](ManagedDirectoryPtr dir)
    {
        if (auto dirEntry = findNodeForData(dir))
            removeNode(dirEntry);
    };

    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_BOOKMARKED) = [this](ManagedDirectoryPtr dir)
    {
        if (auto item = findNodeForData(dir))
            requestItemUpdate(item);
    };

    m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_UNBOOKMARKED) = [this](ManagedDirectoryPtr dir)
    {
        if (auto item = findNodeForData(dir))
            requestItemUpdate(item);
    };
}

AssetDepotTreeModel::~AssetDepotTreeModel()
{}

void AssetDepotTreeModel::addDirNode(const ui::ModelIndex& parentIndex, ManagedDirectory* dir)
{
    auto nodeIndex = addChildNode(parentIndex, dir);

    for (auto& childDir : dir->directories())
        addDirNode(nodeIndex, childDir);
}

bool AssetDepotTreeModel::compare(ManagedItem* a, ManagedItem* b, int colIndex) const
{
    return a->name() < b->name();
}

bool AssetDepotTreeModel::filter(ManagedItem* data, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
{
    return filter.testString(data->name());
}

StringBuf AssetDepotTreeModel::displayContent(ManagedItem* data, int colIndex /*= 0*/) const
{
    StringBuilder ret;

    if (data->cls() == ManagedDirectory::GetStaticClass())
    {
        const auto* dir = static_cast<const ManagedDirectory*>(data);
        if (data == m_depot->root())
        {
            ret << "[img:database]  <depot>";
        }
        /*else if (dir->isFileSystemRoot())
        {
            ret << "[img:package]  " << dir->name();
        }*/
        else
        {
            ret << "[img:folder]  " << dir->name();
            ret.appendf(" [i][color:#888]({})[/color][/i]", dir->fileCount());
        }

        if (dir->isBookmarked())
            ret.append("  [img:star]");
    }

    return ret.toString();
}

//---

END_BOOMER_NAMESPACE(ed)