/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

///----

/// storage entry for a group of configuration variables
/// NOTE: once created exists till deletion of the ConfigStorage (but may be empty)
/// NOTE: the ConfigGroup objects owned by the config system are ETERNAL can be held onto by a pointer
class CORE_CONFIG_API ConfigGroup : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_CONFIG)

public:
    ConfigGroup(ConfigStorage* storage, const StringBuf& name);
    ~ConfigGroup();

    /// get the owning storage
    INLINE ConfigStorage* storage() const { return m_storage; }

    /// get name of the group
    INLINE const StringBuf& name() const { return m_name; }

    /// is this an empty group ?
    INLINE bool empty() const { return m_entries.empty(); }

    /// get all entries
    INLINE const Array<ConfigEntry*>& entries() const { return m_entries.values(); }

    //--

    /// clear values from all entries
    bool clear();

    /// get entry for given name, does not create one if not found
    const ConfigEntry* findEntry(StringView name) const;

    /// remove entry from group (clears all data for the entry)
    bool removeEntry(StringView name);

    /// get entry for given name, creates and empty entry if not found
    ConfigEntry& entry(StringView name);

    /// get value of entry
    StringBuf entryValue(StringView name, const StringBuf& defaultValue = StringBuf::EMPTY()) const;

    //--

    // save group difference
    static void SaveDiff(IFormatStream& f, const ConfigGroup& cur, const ConfigGroup& base);

    // save group content
    static void Save(IFormatStream& f, const ConfigGroup& cur);

private:
    friend class ConfigEntry;

    const ConfigEntry* findEntry_NoLock(StringView name) const;

    ConfigStorage* m_storage;
    StringBuf m_name;
    HashMap<StringBuf, ConfigEntry*> m_entries;
    SpinLock m_lock;

    void modified();
};

///----

END_BOOMER_NAMESPACE()
