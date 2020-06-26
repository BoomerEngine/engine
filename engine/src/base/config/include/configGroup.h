/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace base
{
    namespace config
    {

        ///----

        /// storage entry for a group of configuration variables
        /// NOTE: once created exists till deletion of the ConfigStorage (but may be empty)
        /// NOTE: the ConfigGroup objects owned by the config system are ETERNAL can be held onto by a pointer
        class BASE_CONFIG_API Group : public base::NoCopy
        {
        public:
            Group(Storage* storage, StringID name);
            ~Group();

            /// get the owning storage
            INLINE Storage* storage() const { return m_storage; }

            /// get name of the group
            INLINE StringID name() const { return m_name; }

            /// is this an empty group ?
            INLINE bool empty() const { return m_entries.empty(); }

            /// get all entries
            INLINE const Array<Entry*>& entries() const { return m_entries.values(); }

            //--

            /// clear values from all entries
            bool clear();

            /// get entry for given name, does not create one if not found
            const Entry* findEntry(StringID name) const;

            /// remove entry from group (clears all data for the entry)
            bool removeEntry(StringID name);

            /// get entry for given name, creates and empty entry if not found
            Entry& entry(StringID name);

            /// get value of entry
            StringBuf entryValue(StringID name, const StringBuf& defaultValue = StringBuf::EMPTY()) const;

            //--

            // save group difference
            static void SaveDiff(IFormatStream& f, const Group& cur, const Group& base);

            // save group content
            static void Save(IFormatStream& f, const Group& cur);

        private:
            friend class Entry;

            const Entry* findEntry_NoLock(StringID name) const;

            Storage* m_storage;
            StringID m_name;
            HashMap<StringID, Entry*> m_entries;
            SpinLock m_lock;

            void modified();
        };

        ///----

    } // config
} // base