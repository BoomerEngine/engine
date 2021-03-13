/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "entry.h"
#include "group.h"
#include "storage.h"
#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/stringParser.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigStorage::ConfigStorage()
{}

ConfigStorage::~ConfigStorage()
{
    m_groups.clearPtr();
}

void ConfigStorage::clear()
{
    auto lock  = CreateLock(m_lock);
    for (auto group  : m_groups.values())
        group->clear();
}

void ConfigStorage::modified()
{
    ++m_version;
}

ConfigGroup& ConfigStorage::group(StringView name)
{
    ASSERT_EX(!name.empty(), "ConfigGroup name cannot be empty");

    auto lock  = CreateLock(m_lock);
    return *group_NoLock(name);
}

ConfigGroup* ConfigStorage::group_NoLock(StringView name)
{
    auto group  = m_groups.findSafe(name, nullptr);
    if (!group)
    {
        auto nameStr = StringBuf(name);
        group = new ConfigGroup(this, nameStr);
        m_groups[nameStr] = group;
    }

    return group;
}

const ConfigGroup* ConfigStorage::findGroup(StringView name) const
{
    auto lock  = CreateLock(m_lock);
    return findGroup_NoLock(name);
}

const ConfigGroup* ConfigStorage::findGroup_NoLock(StringView name) const
{
    return m_groups.findSafe(name, nullptr);
}

Array<const ConfigGroup*> ConfigStorage::findAllGroups(StringView groupNameSubString) const
{
    auto lock  = CreateLock(m_lock);
    return findAllGroups_NoLock(groupNameSubString);
}

Array<const ConfigGroup*> ConfigStorage::findAllGroups_NoLock(StringView groupNameSubString) const
{
    Array<const ConfigGroup*> ret;
    for (auto group  : m_groups.values())
        if (group->name().view().beginsWith(groupNameSubString))
            ret.pushBack(group);

    return ret;
}

bool ConfigStorage::removeGroup(StringView name)
{
    auto lock  = CreateLock(m_lock);

    auto group  = m_groups.findSafe(name, nullptr);
    if (group)
        return group->clear();

    return false;
}

bool ConfigStorage::removeEntry(StringView groupName, StringView varName)
{
    auto lock  = CreateLock(m_lock);

    auto group  = m_groups.findSafe(groupName, nullptr);
    if (group)
        return group->removeEntry(varName);

    return false;
}

//--

bool ConfigStorage::Load(StringView txt, ConfigStorage& ret)
{
    StringParser f(txt);
    StringView lastGroup;

    while (f.parseWhitespaces())
    {
        // comment
        if (f.parseKeyword(";"))
        {
            f.parseTillTheEndOfTheLine();
            continue;
        }

        // group name
        if (f.parseKeyword("["))
        {
            StringView groupName;
            if (!f.parseString(groupName, "]"))
            {
                TRACE_ERROR("Failed to parse config group name at line {}, just after group '{}'", f.line(), lastGroup);
                return false;
            }

            // empty group name
            if (groupName.empty())
            {
                TRACE_ERROR("Parsed empty group name at line {}, just after group '{}'", f.line(), lastGroup);
                return false;
            }

            lastGroup = groupName;
            if (!f.parseKeyword("]"))
            {
                TRACE_ERROR("Expected ']' after {} at line {}", groupName, f.line());
                return false;
            }

            // eat rest of the shit
            f.parseTillTheEndOfTheLine();

            // parse values
            auto& group = ret.group(groupName);
            while (f.parseWhitespaces())
            {
                // comment
                if (f.parseKeyword(";"))
                {
                    f.parseTillTheEndOfTheLine();
                    continue;
                }

                // end of shit
                f.push();
                if (f.parseKeyword("["))
                {
                    f.pop();
                    break;
                }
                f.pop();

                // parse identifier
                StringView entryName;
                if (!f.parseString(entryName, "=+-"))
                {
                    TRACE_ERROR("Failed to parse value name at line {} in group {}", f.line(), groupName);
                    return false;
                }

                // delta ?
                bool deltaAdd = f.parseKeyword("+");
                bool deltaSub = f.parseKeyword("-");

                // get the value
                if (!f.parseKeyword("="))
                {
                    TRACE_ERROR("Failed to parse expected '=' at line {}, key {} in group {}", f.line(), entryName, groupName);
                    return false;
                }

                // get the value
                StringView entryValue;
                f.parseTillTheEndOfTheLine(&entryValue);

                // set value
                auto& entry = group.entry(entryName);
                if (deltaAdd)
                    entry.appendValue(StringBuf(entryValue));
                else if (deltaSub)
                    entry.removeValue(StringBuf(entryValue));
                else
                    entry.value(StringBuf(entryValue));
                        
                //TRACE_INFO("{}.{} = {}", groupName, entryName, entryValue);
            }
        }
    }

    // loaded
    return true;
}

void ConfigStorage::Save(IFormatStream& f, const ConfigStorage& cur, const ConfigStorage& base)
{
    // nothing to save if we compare against self
    if (&cur == &base)
        return;

    auto lockA  = CreateLock(cur.m_lock);
    auto lockB  = CreateLock(base.m_lock);

    for (auto curGroup  : cur.m_groups.values())
    {
        if (auto baseGroup  = base.findGroup_NoLock(curGroup->name()))
            ConfigGroup::SaveDiff(f, *curGroup, *baseGroup);
        else
            ConfigGroup::Save(f, *curGroup);
    }
}

//---

END_BOOMER_NAMESPACE()
