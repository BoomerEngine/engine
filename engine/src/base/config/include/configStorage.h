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

        /// config storage contains a PERSISTENT group and entries maps
        /// NOTE: a group or entry that is once created is not deleted until the storage itself is deleted
        class BASE_CONFIG_API Storage : public base::NoCopy
        {
        public:
            Storage();
            ~Storage();

            //--

            // remove all groups and entries
            void clear();

            //--

            // get group, creates an empty new one if not found
            Group& group(StringID name);

            // find group, does not create a new one
            const Group* findGroup(StringID name) const;

            // remove group and all stored entries from the storage
            bool removeGroup(StringID name);

            // remove entry in group
            bool removeEntry(StringID groupName, StringID varName);

            // find all groups starting with given start string
            Array<const Group*> findAllGroups(StringView<char> groupNameSubString) const;

            //---

            // load from a text content
            static bool Load(StringView<char> txt, Storage& ret);

            // save settings to string, filter out settings that are the same as in base
            static void Save(IFormatStream& f, const Storage& cur, const Storage& base);

            // apply values from other storage
            static void Merge(Storage& target, const Storage& from);

            //---

            // get the data version, changed every time we are modified
            INLINE uint32_t currentVersion() const { return m_version.load(); }

        private:
            friend class Group;

            HashMap<StringID, Group*> m_groups;
            std::atomic<uint32_t> m_version;

            SpinLock m_lock;

            Group* group_NoLock(StringID name);
            const Group* findGroup_NoLock(StringID name) const;
            Array<const Group*> findAllGroups_NoLock(StringView<char> groupNameSubString) const;

            void modified();
        };

    } // config
} // base