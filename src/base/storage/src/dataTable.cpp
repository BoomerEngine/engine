/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "dataTable.h"

namespace base
{
    namespace table
    {

        //--

        RTTI_BEGIN_TYPE_STRUCT(Table);
            RTTI_PROPERTY(m_numBuckets);
            RTTI_PROPERTY(m_numEntries);
            RTTI_PROPERTY(m_rootOffset);
            RTTI_PROPERTY(m_contentHash);
            RTTI_PROPERTY(m_data);
        RTTI_END_TYPE();

        //--

        const Table::EntryInfo* Table::findEntryByPath(uint64_t hash) const
        {
            // nothing in the table
            if (m_numBuckets == 0)
                return nullptr;

            // get the bucket index
            ASSERT_EX(base::IsPow2(m_numBuckets), "Invalid bucket count that is not POW2");
            auto bucketIndex = hash & (m_numBuckets - 1);

            // look though the data
            const void* dataBase = m_data.data();
            auto elementOffset = ((const int*)dataBase)[bucketIndex];
            if (elementOffset == 0)
                return nullptr;

            // iterate
            auto entry  = (const EntryInfo*)base::OffsetPtr(dataBase, elementOffset);
            while (entry)
            {
                if (entry->pathHash == hash)
                    break;
                entry = entry->nextHashChild();
            }

            // not found
            return entry;
        }

        void Table::extract(Table&& other)
        {
            m_data = std::move(other.m_data);
            m_contentHash = other.m_contentHash;
            m_rootOffset = other.m_rootOffset;
            m_numBuckets = other.m_numBuckets;
            m_numEntries = other.m_numEntries;
            other.m_contentHash = 0;
            other.m_rootOffset = 0;
            other.m_numBuckets = 0;
            other.m_numEntries = 0;
        }

        bool Table::operator==(const Table& other) const
        {
            return (m_contentHash == other.m_contentHash) && (m_data.size() == other.m_data.size());
        }

        bool Table::operator!=(const Table& other) const
        {
            return !operator==(other);
        }

        //--

    } // table
} // base


