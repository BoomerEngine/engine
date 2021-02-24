/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "dataTable.h"
#include "base/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::storage)

///-------------

/// helper class to build the Table
class BASE_STORAGE_API TableBuilder : public base::NoCopy
{
public:
    TableBuilder();

    // clear existing content, allows to build another table
    void clear();

    // open an entry under current parent, name can be given (allows to find the entry by name)
    // NOTE: elements in array should not be named
    // NOTE: named and unnamed entries cannot be mixed in a node
    // NOTE: can fail if we do something illegal
    bool beginEntry(const char* nameIfGiven);

    // close entry
    void endEntry();

    // set text value of an entry
    bool valueString(const char* text);

    // set number value of an entry
    bool valueNumber(double number);

    // set boolean value an an entry
    bool valueBool(bool val);

    //--

    /// extract current data into a table
    void extract(Table& outTable) const;

private:
    struct TempString
    {
        int m_storageOffset; // set by export()
        const char *m_str;

        INLINE TempString()
            : m_str(nullptr)
            , m_storageOffset(0)
        {}

        INLINE uint32_t payloadSize() const
        {
			return range_cast<uint32_t>(strlen(m_str) + 1);
        }

        INLINE void writePayload(void* mem) const
        {
            memcpy(mem, m_str, strlen(m_str) + 1);
        }
    };

    struct TempValue
    {
        Table::DataType m_type;
        uint64_t m_hash;
        int m_storageOffset; // set by export()

        union
        {
            float m_number;
            bool m_bool;
            const char *m_str;
        } val;

        INLINE TempValue()
            : m_type(Table::DataType::Number)
            , m_storageOffset(0)
        {
            val.m_number = 0.0;
        }

        INLINE uint32_t payloadSize() const
        {
            switch (m_type)
            {
                case Table::DataType::String:
					return range_cast<uint32_t>(strlen(val.m_str) + 1);

                case Table::DataType::Number:
                    return sizeof(float);

                case Table::DataType::Boolean:
                    return 1;
            }

			return 0;
        }

        INLINE void writePayload(void* pos)
        {
            switch (m_type)
            {
                case Table::DataType::String:
                    memcpy(pos, val.m_str, strlen(val.m_str) + 1);
                    break;

                case Table::DataType::Number:
                    *(float*)pos = val.m_number;
                    break;

                case Table::DataType::Boolean:
                    *(uint8_t*)pos = val.m_bool ? 1 : 0;
                    break;
            }
        }
    };

    struct TempNode;

    struct TempLink
    {
        TempNode* m_node;
        TempLink* m_next;

        INLINE TempLink()
            : m_node(nullptr)
            , m_next(nullptr)
        {}
    };

    struct TempNode
    {
        uint64_t m_pathHash;
        uint64_t m_contentHash;

        const TempValue* m_value;
        const TempString* m_name; // only for named nodes
        TempLink* m_child;
        TempLink* m_last;
        uint32_t m_numChildren;
        bool m_hasArrayChildren:1;
        bool m_hasNamedChildren:1;
        bool m_hasValue:1;

        int m_storageOffset;

        INLINE TempNode()
            : m_pathHash(0)
            , m_contentHash(0)
            , m_child(nullptr)
            , m_last(nullptr)
            , m_value(nullptr)
            , m_numChildren(0)
            , m_storageOffset(0)
            , m_hasArrayChildren(0)
            , m_hasNamedChildren(0)
            , m_hasValue(0)
        {}

        INLINE uint32_t payloadSize() const
        {
            return sizeof(Table::EntryInfo) + (m_numChildren * sizeof(uint32_t));
        }

        INLINE void writePayload(void* ptr) const
        {
            // write basic node info
            auto info  = (Table::EntryInfo*)ptr;
            info->pathHash = m_pathHash;
            info->contentHash = m_contentHash;
            info->nameOffset = m_name ? (m_name->m_storageOffset - m_storageOffset) : 0;
            info->dataOffset = m_value ? (m_value->m_storageOffset - m_storageOffset) : 0;
            info->dataType = m_value ? m_value->m_type : Table::DataType::None;
            info->numChildren = m_numChildren;
            info->nextHashOffset = 0;

            // write offsets to child nodes
            auto childOffsets  = (int*)((char*)ptr + sizeof(Table::EntryInfo));
            uint32_t writeIndex = 0;
            for (auto link  = m_child; link != nullptr; link = link->m_next)
            {
                childOffsets[writeIndex] = link->m_node->m_storageOffset - m_storageOffset;
            }

            ASSERT(writeIndex == m_numChildren);
        }
    };

    mem::LinearAllocator m_mem;
    uint32_t m_numTotalNodes;
    TempNode* m_root;

    Array<TempNode*> m_nodeStack;

    HashMap<uint64_t, const TempString*> m_stringMap;
    Array<TempString*> m_stringList;

    HashMap<uint64_t, const TempValue*> m_valueMap;
    Array<TempValue*> m_valueList;

    static uint64_t CalcValueHash(const TempValue& value);

    const TempString* mapString(const char* str);
    const TempValue* mapValue(const TempValue& value);

    void placeNodes(TempNode* node, Array<TempNode*>& outNodeList, uint32_t& dataOffset) const;
};

///-------------

END_BOOMER_NAMESPACE(base::storage)
