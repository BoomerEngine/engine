/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace base
{
    namespace replication
    {

        //--

        /// repository of all packing models created for structures
        class BASE_REPLICATION_API DataModelRepository : public IReferencable
        {
        public:
            DataModelRepository();
            ~DataModelRepository();

            //--

            /// ask for a data model for given compound type (struct/class)
            const DataModel* buildModelForType(Type type);

            /// ask for a data model for given function (for RPC/messages)
            const DataModel* buildModelForFunction(const rtti::Function* func);

            //--

        private:
            Mutex m_lock;

            HashMap<StringID, DataModel*> m_compoundModels;
            HashMap<StringBuf, DataModel*> m_functionModels;
        };

        //--

    } // replication
} // base
