/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "managedDepot.h"
#include "base/ui/include/uiSimpleTreeModel.h"

namespace ed
{
    //--

    class AssetDepotTreeModel : public ui::SimpleTreeModel<ManagedItem*, ManagedItem*>, public IManagedDepotListener
    {
    public:
        AssetDepotTreeModel(ManagedDepot* depot);
        virtual ~AssetDepotTreeModel();

    private:
        ManagedDepot* m_depot;

        virtual bool compare(ManagedItem* a, ManagedItem* b, int colIndex) const override final;
        virtual bool filter(ManagedItem* data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(ManagedItem* data, int colIndex = 0) const override final;


        // depot::IManagedDepotListener
        virtual void managedDepotEvent(ManagedItem* item, ManagedDepotEvent eventType) override final;

        void addDirNode(const ui::ModelIndex& parent, ManagedDirectory* dir);
    };

    //--

} // ed