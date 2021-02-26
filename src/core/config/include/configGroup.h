/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE_EX(config)

///----

/// storage entry for a group of configuration variables
/// NOTE: once created exists till deletion of the ConfigStorage (but may be empty)
/// NOTE: the ConfigGroup objects owned by the config system are ETERNAL can be held onto by a pointer
class CORE_CONFIG_API Group : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_CONFIG)

public:
    Group(Storage* storage, const StringBuf& name);
    ~Group();

    /// get the owning storage
    INLINE Storage* storage() const { return m_storage; }

    /// get name of the group
    INLINE const StringBuf& name() const { return m_name; }

    /// is this an empty group ?
    INLINE bool empty() const { return m_entries.empty(); }

    /// get all entries
    INLINE const Array<Entry*>& entries() const { return m_entries.values(); }

    //--

    /// clear values from all entries
    bool clear();

    /// get entry for given name, does not create one if not found
    const Entry* findEntry(StringView name) const;

    /// remove entry from group (clears all data for the entry)
    bool removeEntry(StringView name);

    /// get entry for given name, creates and empty entry if not found
    Entry& entry(StringView name);

    /// get value of entry
    StringBuf entryValue(StringView name, const StringBuf& defaultValue = StringBuf::EMPTY()) const;

    //--

    // save group difference
    static void SaveDiff(IFormatStream& f, const Group& cur, const Group& base);

    // save group content
    static void Save(IFormatStream& f, const Group& cur);

private:
    friend class Entry;

    const Entry* findEntry_NoLock(StringView name) const;

    Storage* m_storage;
    StringBuf m_name;
    HashMap<StringBuf, Entry*> m_entries;
    SpinLock m_lock;

    void modified();
};

///----

END_BOOMER_NAMESPACE_EX(config)
