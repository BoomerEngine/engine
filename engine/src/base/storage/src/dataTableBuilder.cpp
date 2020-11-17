/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "dataTable.h"
#include "dataTableBuilder.h"

namespace base
{
    namespace table
    {

        TableBuilder::TableBuilder()
            : m_mem(POOL_TEMP)
            , m_numTotalNodes(0)
            , m_root(nullptr)
        {}

        void TableBuilder::clear()
        {
            // reset allocator
            m_numTotalNodes = 0;
            m_stringList.reset();
            m_stringMap.reset();
            m_valueList.reset();
            m_valueMap.reset();
            m_mem.clear();

            // push the root node on the stack
            // NOTE: the root node is NOT exported thus we don't increment the numNodes
            m_root = m_mem.create<TempNode>();
            m_root->m_pathHash = CRC64Init;
            m_nodeStack.pushBack(m_root);
        }

        const TableBuilder::TempString* TableBuilder::mapString(const char* str)
        {
            if (!str || !*str)
                return nullptr;

            auto strHash = StringView(str).calcCRC64();
            const TempString* ptr = nullptr;
            if (m_stringMap.find(strHash, ptr))
                return ptr;

            auto newStr  = m_mem.create<TempString>();
            newStr->m_storageOffset = 0;
            newStr->m_str = m_mem.strcpy(str);

            m_stringList.pushBack(newStr);
            m_stringMap[strHash] = newStr;
            return ptr;
        }

        bool TableBuilder::beginEntry(const char* nameIfGiven)
        {
            // check parent, note: we cannot add named nodes to a parent that already have array nodes or value
            auto top  = m_nodeStack.back();
            if (nameIfGiven)
            {
                if (top->m_hasArrayChildren || top->m_hasValue)
                    return false;
            }
            else
            {
                if (top->m_hasNamedChildren || top->m_hasValue)
                    return false;
            }

            // create node and assign name
            auto node  = m_mem.create<TempNode>();
            node->m_name = mapString(nameIfGiven);

            // count nodes in parent
            if (nameIfGiven)
                node->m_hasNamedChildren = true;
            else
                node->m_hasArrayChildren = true;
            node->m_numChildren += 1;

            // compute name of the node
            if (nameIfGiven)
            {
                if (top != m_root)
                    node->m_pathHash = StringView(base::TempString(".{}", nameIfGiven)).calcCRC64();
                else
                    node->m_pathHash = StringView(base::TempString("{}", nameIfGiven)).calcCRC64();
            }
            else
            {
                node->m_pathHash = StringView(base::TempString("[{}]", node->m_numChildren - 1)).calcCRC64();
            }

            // create node link
            auto link  = m_mem.create<TempLink>();
            link->m_node = node;
            link->m_next = nullptr;

            // link with parent
            if (top->m_child == nullptr)
            {
                top->m_child = link;
                top->m_last = link;
            }
            else
            {
                top->m_last->m_next = link;
                top->m_last = link;
            }

            // add to stack
            m_numTotalNodes += 1;
            m_nodeStack.pushBack(node);
            return true;
        }

        void TableBuilder::endEntry()
        {
            ASSERT(m_nodeStack.size() > 1);

            // calculate node hash
            auto top  = m_nodeStack.back();
            CRC64 crc;
            crc << top->m_name->m_str;
            crc << top->m_numChildren;
            if (top->m_value)
                crc << top->m_value->m_hash;
            else
                crc << 0;
            for (auto child  = top->m_child; child; child = child->m_next)
                crc << child->m_node->m_contentHash;

            // set node content hash that identifies this data in a unique way in the whole tables
            top->m_contentHash = crc.crc();

            // remove from stack
            m_nodeStack.popBack();
        }

        uint64_t TableBuilder::CalcValueHash(const TempValue& value)
        {
            CRC64 crc;

            crc << (uint8_t)value.m_type;
            switch (value.m_type)
            {
                case Table::DataType::String:
                    crc << value.val.m_str;
                    break;

                case Table::DataType::Number:
                    crc << value.val.m_number;
                    break;

                case Table::DataType::Boolean:
                    crc << value.val.m_bool;
                    break;
            }
            return crc.crc();
        }

        const TableBuilder::TempValue* TableBuilder::mapValue(const TempValue& valueData)
        {
            auto hash = CalcValueHash(valueData);

            const TableBuilder::TempValue* value = nullptr;
            if (m_valueMap.find(hash, value))
                return value;

            auto newValue  = m_mem.create<TempValue>();
            newValue->m_hash = hash;
            newValue->m_storageOffset = 0;
            newValue->m_type = valueData.m_type;

            switch (valueData.m_type)
            {
                case Table::DataType::String:
                    newValue->val.m_str = m_mem.strcpy(valueData.val.m_str);
                    break;

                case Table::DataType::Number:
                    newValue->val.m_number = valueData.val.m_number;
                    break;

                case Table::DataType::Boolean:
                    newValue->val.m_bool = valueData.val.m_bool;
                    break;
            }

            m_valueMap[hash] = newValue;
            m_valueList.pushBack(newValue);
            return newValue;
        }

        bool TableBuilder::valueString(const char* text)
        {
            auto top  = m_nodeStack.back();
            if (top->m_hasNamedChildren || top->m_hasArrayChildren || top->m_hasValue)
                return false;

            TempValue val;
            val.m_type = Table::DataType::String;
            val.val.m_str = text;

            top->m_value = mapValue(val);
            top->m_hasValue = true;
            return true;
        }

        bool TableBuilder::valueNumber(double number)
        {
            auto top  = m_nodeStack.back();
            if (top->m_hasNamedChildren || top->m_hasArrayChildren || top->m_hasValue)
                return false;

            TempValue val;
            val.m_type = Table::DataType::String;
            val.val.m_number = (float)number;

            top->m_value = mapValue(val);
            top->m_hasValue = true;
            return true;
        }

        bool TableBuilder::valueBool(bool boolean)
        {
            auto top  = m_nodeStack.back();
            if (top->m_hasNamedChildren || top->m_hasArrayChildren || top->m_hasValue)
                return false;

            TempValue val;
            val.m_type = Table::DataType::Boolean;
            val.val.m_bool = boolean;

            top->m_value = mapValue(val);
            top->m_hasValue = true;
            return true;
        }

        //--

        void TableBuilder::placeNodes(TableBuilder::TempNode* node, Array<TableBuilder::TempNode*>& outNodeList, uint32_t& dataOffset) const
        {
            // place node
            node->m_storageOffset = dataOffset;
            dataOffset += node->payloadSize();

            // add to list to write
            outNodeList.pushBack(node);

            // visit children, this makes sure children are stored after the parent
            for (auto link  = node->m_child; link != nullptr; link = link->m_next)
                placeNodes(link->m_node, outNodeList, dataOffset);
        }

        void TableBuilder::extract(Table& outTable) const
        {
            ScopeTimer timer;

            // reset table data
            outTable.m_data.reset();
            outTable.m_contentHash = 0;
            outTable.m_rootOffset = 0;

            // calculate placement for everything in the file
            uint32_t dataOffset = 0;

            // first, prepare a bucket table, we need at least +30% buckets than nodes for faster searches
            // TODO: perfect hashing ?
            auto numBuckets = NextPow2((uint32_t)(m_numTotalNodes * 130 / 100));
            outTable.m_numBuckets = std::min<uint32_t>(1 << 22, numBuckets); // keep to sane limit
            TRACE_INFO("Building from {} nodes ({} buckets)", m_numTotalNodes, outTable.m_numBuckets);
            dataOffset += sizeof(uint32_t) * numBuckets;

            // next we add all the values
            uint32_t valuesStartOffset = dataOffset;
            for (auto val  : m_valueList)
            {
                val->m_storageOffset = dataOffset;
                dataOffset += val->payloadSize();
            }
            TRACE_INFO("Found {} unique values ({})", m_valueList.size(), MemSize(dataOffset - valuesStartOffset));

            // next we add all the strings
            uint32_t stringsStartOffset = dataOffset;
            for (auto val  : m_stringList)
            {
                val->m_storageOffset = dataOffset;
                dataOffset += val->payloadSize();
            }
            TRACE_INFO("Found {} unique strings ({})", m_stringList.size(), MemSize(dataOffset - stringsStartOffset));

            // now place the nodes
            uint32_t nodesStartOffset = dataOffset;
            Array<TempNode*> nodeList;
            placeNodes(m_root, nodeList, dataOffset);
            TRACE_INFO("Found {} unique nodes ({})", nodeList.size(), MemSize(dataOffset - nodesStartOffset));

            // allocate the data buffer
            outTable.m_rootOffset = nodesStartOffset;
            outTable.m_contentHash = nodeList.empty() ? 0 : nodeList[0]->m_contentHash;
            outTable.m_numEntries = nodeList.size();
            outTable.m_data = Buffer::Create(POOL_TABLE_DATA, dataOffset);

            // clear buckets
            auto writeBuffer = outTable.m_data.data();

            // write values
            for (auto val  : m_valueList)
            {
                auto mem  = base::OffsetPtr(writeBuffer, val->m_storageOffset);
                val->writePayload(mem);
            }

            // write strings
            for (auto val  : m_stringList)
            {
                auto mem  = base::OffsetPtr(writeBuffer, val->m_storageOffset);
                val->writePayload(mem);
            }

            // write nodes
            for (auto val  : nodeList)
            {
                auto mem  = base::OffsetPtr(writeBuffer, val->m_storageOffset);
                val->writePayload(mem);
            }

            // insert nodes to hash buckets
            auto buckets  = (int*)writeBuffer;
            for (auto val  : nodeList)
            {
                auto bucketIndex = val->m_pathHash % outTable.m_numBuckets;

                auto writtenNodePtr  = (Table::EntryInfo*)base::OffsetPtr(writeBuffer, val->m_storageOffset);
                if (0 != buckets[bucketIndex])
                {
                    auto nextHashNodeOffset = buckets[bucketIndex];
                    writtenNodePtr->nextHashOffset = (nextHashNodeOffset - val->m_storageOffset);
                }

                buckets[bucketIndex] = val->m_storageOffset;
            }

            // take some statistics from the hashmap, this also validates that we reach all nodes
            uint32_t numEmptyBuckets = 0;
            uint32_t maxChainLength = 0;
            uint32_t averageChainLengthSum = 0;
            uint32_t averageChainLengthDiv = 0;
            uint32_t numHashedNodes = 0;
            for (uint32_t i=0; i<outTable.m_numBuckets; ++i)
            {
                auto offset = buckets[i];
                if (offset == 0)
                {
                    numEmptyBuckets += 1;
                }
                else
                {
                    uint32_t length = 0;
                    auto node  = (const Table::EntryInfo*)writeBuffer;
                    while (node != nullptr)
                    {
                        length += 1;
                        node = node->nextHashChild();
                        numHashedNodes += 1;
                    }

                    maxChainLength = std::max<uint32_t>(maxChainLength, length);
                    averageChainLengthDiv += 1;
                    averageChainLengthSum += length;
                }
            }
            ASSERT(numHashedNodes == nodeList.size());
            if (averageChainLengthDiv != 0)
                TRACE_INFO("Hashmap has {} empty buckets, {} max chain, {} average chain length", numEmptyBuckets, maxChainLength, (float)averageChainLengthSum / (float)averageChainLengthDiv);

            // we are done
            TRACE_INFO("Compiled data table from {} nodes in {}", numHashedNodes, TimeInterval(timer.timeElapsed()));
        }

    } // table
} // base


