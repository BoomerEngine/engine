/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "configEntry.h"
#include "configGroup.h"
#include "configStorage.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"

namespace base
{
    namespace config
    {
        //---

        Storage::Storage()
        {}

        Storage::~Storage()
        {
            m_groups.clearPtr();
        }

        void Storage::clear()
        {
            auto lock  = CreateLock(m_lock);
            for (auto group  : m_groups.values())
                group->clear();
        }

        void Storage::modified()
        {
            ++m_version;
        }

        Group& Storage::group(StringView name)
        {
            ASSERT_EX(!name.empty(), "Group name cannot be empty");

            auto lock  = CreateLock(m_lock);
            return *group_NoLock(name);
        }

        Group* Storage::group_NoLock(StringView name)
        {
            auto group  = m_groups.findSafe(name, nullptr);
            if (!group)
            {
                auto nameStr = StringBuf(name);
                group = MemNewPool(POOL_CONFIG, Group, this, nameStr);
                m_groups[nameStr] = group;
            }

            return group;
        }

        const Group* Storage::findGroup(StringView name) const
        {
            auto lock  = CreateLock(m_lock);
            return findGroup_NoLock(name);
        }

        const Group* Storage::findGroup_NoLock(StringView name) const
        {
            return m_groups.findSafe(name, nullptr);
        }

        Array<const Group*> Storage::findAllGroups(StringView groupNameSubString) const
        {
            auto lock  = CreateLock(m_lock);
            return findAllGroups_NoLock(groupNameSubString);
        }

        Array<const Group*> Storage::findAllGroups_NoLock(StringView groupNameSubString) const
        {
            Array<const Group*> ret;
            for (auto group  : m_groups.values())
                if (group->name().view().beginsWith(groupNameSubString))
                    ret.pushBack(group);

            return ret;
        }

        bool Storage::removeGroup(StringView name)
        {
            auto lock  = CreateLock(m_lock);

            auto group  = m_groups.findSafe(name, nullptr);
            if (group)
                return group->clear();

            return false;
        }

        bool Storage::removeEntry(StringView groupName, StringView varName)
        {
            auto lock  = CreateLock(m_lock);

            auto group  = m_groups.findSafe(groupName, nullptr);
            if (group)
                return group->removeEntry(varName);

            return false;
        }

        //--

        bool Storage::Load(StringView txt, Storage& ret)
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

        void Storage::Save(IFormatStream& f, const Storage& cur, const Storage& base)
        {
            // nothing to save if we compare against self
            if (&cur == &base)
                return;

            auto lockA  = base::CreateLock(cur.m_lock);
            auto lockB  = base::CreateLock(base.m_lock);

            for (auto curGroup  : cur.m_groups.values())
            {
                if (auto baseGroup  = base.findGroup_NoLock(curGroup->name()))
                    Group::SaveDiff(f, *curGroup, *baseGroup);
                else
                    Group::Save(f, *curGroup);
            }
        }

        //---

    } // config
} // storage