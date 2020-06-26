/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

namespace base
{
    namespace table
    {

        ///-------------

        class TableBuilder;

        /// table of processed configuration entries
        /// NOTE: the values are not yet resolved into more complex types and are stored RAW
        class BASE_STORAGE_API Table
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(Table);

        public:
            INLINE Table() {};
            INLINE Table(const Table& other) = default;
            INLINE Table(Table&& other) = default;
            INLINE Table& operator=(const Table& other) = default;
            INLINE Table& operator=(Table&& other) = default;

            enum class DataType : uint8_t
            {
                None=0,
                String=1,
                Number=2, // double!
                Boolean=3,
            };

#pragma pack(push)
#pragma pack(1)
            struct EntryInfo
            {
                // get name of the node
                INLINE const char* name() const
                {
                    return nameOffset ? (const char*)base::OffsetPtr(this, nameOffset) : "";
                }

                // get node data, if any
                INLINE const void* data() const
                {
                    return dataOffset ? (const void*)base::OffsetPtr(this, dataOffset) : nullptr;
                }

                // get pointer to n-th child of this node
                // NOTE: the index must < numChildren
                INLINE const EntryInfo* child(uint32_t index) const
                {
                    ASSERT_EX(index < numChildren, "Invalid node child index");
                    auto offsetTable  = (const uint32_t*)(this + 1);
                    return (const EntryInfo*)base::OffsetPtr(this, offsetTable[index]);
                }

                // get number of children in this node
                INLINE const EntryInfo* nextHashChild() const
                {
                    return nextHashOffset ? (const EntryInfo*)base::OffsetPtr(this, nextHashOffset) : nullptr;
                }

                //--

                uint64_t pathHash = 0; // path to this node so far, always valid
                uint64_t contentHash = 0; // hash of the unique node's content (including children hashes)
                int nameOffset = -1; // verbatim name of this node, relative to node entry
                int dataOffset = -1; // offset to data in the data block
                DataType dataType = DataType::None; // type of stored data
                uint32_t numChildren = 0; // general number of children
                int nextHashOffset = -1; // offset to next child in hash bucket
            };
#pragma pack(pop)

            // is this an empty table ?
            INLINE bool empty() const { return !m_data; }

            // get hash of the content in this table, can be used to detect changes (or not)
            INLINE uint64_t contentHash() const { return m_contentHash; }

            // get root node of the document, allows to explore content manually
            INLINE const EntryInfo* root() const { return (const EntryInfo*)base::OffsetPtr(m_data.data(), m_rootOffset); }

            // get size of data
            INLINE uint32_t size() const { return m_data.size(); }

            // get total number of entries in the table
            INLINE uint32_t numEntries() const { return m_numEntries; }

            // find entry for given path
            // NOTE: we always return something as we may reference entries that are not yet in file
            const EntryInfo* findEntryByPath(uint64_t hash) const;

            //---

            // Suck data from other table
            void extract(Table&& other);

            //---

            // Test for equality
            bool operator==(const Table& other) const;
            bool operator!=(const Table& other) const;

        private:
            Buffer m_data;

            uint64_t m_contentHash = 0;
            uint32_t m_rootOffset = 0;
            uint32_t m_numBuckets = 0;
            uint32_t m_numEntries = 0;

            friend class TableBuilder;
        };

        ///-------------

    } // table
} // base
