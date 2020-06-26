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

namespace ed
{   

    //---

    AssetDepotTreeModel::AssetDepotTreeModel(ManagedDepot* depot)
        : m_depot(depot)
    {
        if (auto rootDirectory = depot->root())
            addDirNode(ui::ModelIndex(), rootDirectory);
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

    base::StringBuf AssetDepotTreeModel::displayContent(ManagedItem* data, int colIndex /*= 0*/) const
    {
        base::StringBuilder ret;

        if (data->cls() == ManagedDirectory::GetStaticClass())
        {
            const auto* dir = static_cast<const ManagedDirectory*>(data);
            if (data == m_depot->root())
            {
                ret << "[img:database]  <depot>";
            }
            else if (dir->isFileSystemRoot())
            {
                ret << "[img:package]  " << dir->name();
            }
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

    void AssetDepotTreeModel::managedDepotEvent(ManagedItem* depotItem, ManagedDepotEvent eventType)
    {
        if (eventType == ManagedDepotEvent::FileCreated || eventType == ManagedDepotEvent::FileDeleted)
        {
            if (auto parentDir = depotItem->parentDirectory())
            {
                if (auto item = this->findNodeForData(parentDir))
                {
                    requestItemUpdate(item, ui::ItemUpdateModeBit::ItemAndParents);
                }
            }
        }
        else if (eventType == ManagedDepotEvent::DirCreated)
        {
            if (auto parentDir = depotItem->parentDirectory())
            {
                if (auto parentEntry = findNodeForData(parentDir))
                {
                    addChildNode(parentEntry, depotItem);
                }
            }
        }
        else if (eventType == ManagedDepotEvent::DirDeleted)
        {
            if (auto dirEntry = findNodeForData(depotItem))
            {
                removeNode(dirEntry);
            }
        }
        else if (eventType == ManagedDepotEvent::DirBookmarkChanged)
        {
            if (auto item = findNodeForData(depotItem))
            {
                requestItemUpdate(item);
            }
        }
    }

    //---

} // ed