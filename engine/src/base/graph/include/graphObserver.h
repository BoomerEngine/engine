/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

namespace base
{
    namespace graph
    {

        //---

        /// graph view observer
        class BASE_GRAPH_API IGraphObserver : public NoCopy
        {
        public:
            virtual ~IGraphObserver();

            /// block was added to the graph
            virtual void handleBlockAdded(Block* block) = 0;

            /// block was removed from the graph
            virtual void handleBlockRemoved(Block* block) = 0;

            /// block's style has changed
            virtual void handleBlockStyleChanged(Block* block) = 0;

            /// block's layout (sockets) have changed
            virtual void handleBlockLayoutChanged(Block* block) = 0;

            /// connections on block have changed
            virtual void handleBlockConnectionsChanged(Block* block) = 0;
        };

        //---

    } // graph
} // base
