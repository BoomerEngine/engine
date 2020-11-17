/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringBuf.h"
#include "stringID.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    //---

    namespace prv
    {

        // String data manager
     // Manages the string hash map. Allocates the strings.
        class StringDataMgr : public ISingleton
        {
            DECLARE_SINGLETON(StringDataMgr);

        public:
            StringDataMgr()
                : m_localAllocator(POOL_KEYS)
            {
                memset(m_buckets, 0, sizeof(m_buckets));
                m_entries.reserve(65536);
                m_entries.pushBack(nullptr);
            }

            StringIDIndex allocString(StringView buf)
            {
                if (!buf)
                    return 0;

                auto hash = StringView::CalcHash(buf);
                auto bucketIndex = hash % HASH_SIZE;

                auto lock = CreateLock(m_lock);

                // find
                auto entry  = m_buckets[bucketIndex];
                while (entry)
                {
                    if (entry->hash == hash && buf == entry->txt)
                        return entry->index;
                    entry = entry->next;
                }

                // allocate storage
                entry = (StringEntry*)m_localAllocator.alloc(sizeof(StringEntry) + buf.length(), 1);
                memcpy(entry->txt, buf.data(), buf.length());
                entry->txt[buf.length()] = 0;
                entry->hash = hash;
                entry->index = m_entries.size();
                entry->length = buf.length();
                entry->next = m_buckets[bucketIndex];
                m_buckets[bucketIndex] = entry;

                // place in entry list
                m_entries.pushBack(entry);
                return entry->index;
            }

            StringIDIndex findString(StringView buf) const
            {
                if (!buf)
                    return 0;

                auto hash = StringView::CalcHash(buf);
                auto bucketIndex = hash % HASH_SIZE;

                auto lock = CreateLock(m_lock);

                // find
                auto entry  = m_buckets[bucketIndex];
                while (entry)
                {
                    if (entry->hash == hash && buf == entry->txt) 
                        return entry->index;
                    entry = entry->next;
                }

                // not found
                return 0;
            }

            StringView view(StringIDIndex id) const
            {
                if (!id)
                    return StringView();

                //auto lock = CreateLock(m_lock);
                if (id >= m_entries.size())
                    return StringView();

                return StringView(m_entries[id]->txt, m_entries[id]->length);
            }

        private:
            //--

            static const uint32_t HASH_SIZE = 8192;

            //--

            struct StringEntry : NoCopy
            {
                StringEntry* next = nullptr;
                StringIDIndex index;
                uint64_t hash; // calculated text hash value
                uint32_t length;
                char txt[1];
            };

            SpinLock m_lock;

            StringEntry* m_buckets[HASH_SIZE];

            Array<StringEntry*> m_entries;

            mem::LinearAllocator m_localAllocator;

            //--

            virtual void deinit() override
            {
                m_entries.clear();
                m_localAllocator.clear();
            }
        };

    } // prv

    //---

    void StringID::set(StringView txt)
    {
        indexValue = prv::StringDataMgr::GetInstance().allocString(txt);
    }

    const char* StringID::debugString() const
    {
        return indexValue ? prv::StringDataMgr::GetInstance().view(indexValue).data() : "";
    }

    const char* StringID::DebugString(StringIDIndex id)
    {
        return prv::StringDataMgr::GetInstance().view(id).data();
    }

    StringView StringID::View(StringIDIndex id)
    {
        return prv::StringDataMgr::GetInstance().view(id);
    }

    StringID GEmptyStringID;

    StringID StringID::EMPTY()
    {
        return GEmptyStringID;
    }

    StringID StringID::Find(StringView txt)
    {
        StringID ret;
        ret.indexValue = prv::StringDataMgr::GetInstance().findString(txt);
        return ret;
    }

    //---

} // base