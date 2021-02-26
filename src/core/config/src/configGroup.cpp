/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "configStorage.h"
#include "configGroup.h"
#include "configEntry.h"

BEGIN_BOOMER_NAMESPACE_EX(config)

//---

Group::Group(Storage* storage, const StringBuf& name)
    : m_storage(storage)
    , m_name(name)
{
}

Group::~Group()
{
    m_entries.clearPtr();
}

bool Group::clear()
{
    auto lock  = CreateLock(m_lock);

    bool wasModified = false;
    for (auto entry  : m_entries.values())
        wasModified |= entry->clear();

    return wasModified;
}

StringBuf Group::entryValue(StringView name, const StringBuf& defaultValue/*= ""*/) const
{
    if (auto entry  = findEntry(name))
        return entry->value();
    return defaultValue;
}

const Entry* Group::findEntry(StringView name) const
{
    auto lock  = CreateLock(m_lock);
    return findEntry_NoLock(name);
}

const Entry* Group::findEntry_NoLock(StringView name) const
{
    return m_entries.findSafe(name, nullptr);
}

Entry& Group::entry(StringView name)
{
    auto lock  = CreateLock(m_lock);

    ASSERT_EX(!name.empty(), "Config entry name cannot be empty");

    auto entry  = m_entries.findSafe(name, nullptr);
    if (!entry)
    {
        auto nameStr = StringBuf(name);
        entry = new Entry(this, nameStr);
        m_entries[nameStr] = entry;
        // NOTE: we do not mark a group as modified since empty entry will NOT be saved any way
    }

    return *entry;
}

bool Group::removeEntry(StringView name)
{
    auto lock  = CreateLock(m_lock);

    auto entry  = m_entries.findSafe(name, nullptr);
    if (!entry)
    {
        if (entry->clear())
        {
            modified();
            return true;
        }
    }

    return false;
}

void Group::modified()
{
    m_storage->modified();
}

void Group::SaveDiff(IFormatStream& f, const Group& cur, const Group& base)
{
    auto lockA  = CreateLock(cur.m_lock);
    auto lockB  = CreateLock(base.m_lock);

    bool headerPrinted = false;

    for (auto entry  : cur.m_entries.values())
    {
        // compare against the base
        auto baseEntry  = base.findEntry_NoLock(entry->name());
        if (baseEntry)
        {
            if (Entry::Equal(*entry, *baseEntry))
                continue;
        }

        // we will be writing the value, print the group header
        if (!headerPrinted)
        {
            headerPrinted = true;
            f.appendf("[{}]\n", cur.m_name);
        }

        // print the value difference
        if (baseEntry)
            Entry::PrintDiff(f, *entry, *baseEntry);
        else
            Entry::Print(f, *entry);
    }

    // print separator
    if (headerPrinted)
        f.append("\n");
}

void Group::Save(IFormatStream& f, const Group& cur)
{
    auto lockA  = CreateLock(cur.m_lock);

    bool headerPrinted = false;
    for (auto entry  : cur.m_entries.values())
    {
        // we will be writing the value, print the group header
        if (!headerPrinted)
        {
            headerPrinted = true;
            f.appendf("[{}]\n", cur.m_name);
        }

        // print the value difference
        Entry::Print(f, *entry);
    }

    // print separator
    if (headerPrinted)
        f.append("\n");
}

//---

END_BOOMER_NAMESPACE_EX(config)
