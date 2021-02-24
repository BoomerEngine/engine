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
#include "configSystem.h"

BEGIN_BOOMER_NAMESPACE(base::config)

//--

namespace prv
{

    class ConfigStorage : public ISingleton
    {
        DECLARE_SINGLETON(ConfigStorage);

    public:
        ConfigStorage()
        {
            m_storage = new Storage;
        }

        virtual void deinit() override
        {
            delete m_storage;
        }

        Storage* m_storage;
    };
}

//--

Storage& RawStorageData()
{
    return *prv::ConfigStorage::GetInstance().m_storage;
}

//--

Group& MakeGroup(StringView name)
{
    return prv::ConfigStorage::GetInstance().m_storage->group(name);
}

const Group* FindGroup(StringView name)
{
    return prv::ConfigStorage::GetInstance().m_storage->findGroup(name);
}

Array<const Group*> FindAllGroups(StringView groupNameSubString)
{
    return prv::ConfigStorage::GetInstance().m_storage->findAllGroups(groupNameSubString);
}

Entry& MakeEntry(StringView groupName, StringView entryName)
{
    return MakeGroup(groupName).entry(entryName);
}

const Entry* FindEntry(StringView groupName, StringView entryName)
{
    if (auto group = FindGroup(groupName))
        return group->findEntry(entryName);

    return nullptr;
}

const Array<StringBuf> Values(StringView groupName, StringView entryName)
{
    if (auto entry = FindEntry(groupName, entryName))
        return entry->values();

    return Array<StringBuf>();
}

StringBuf Value(StringView groupName, StringView entryName, const StringBuf& defaultValue)
{
    if (auto entry = FindEntry(groupName, entryName))
        return entry->value();

    return defaultValue;
}

int ValueInt(StringView groupName, StringView entryName, int defaultValue/* = 0*/)
{
    if (auto entry = FindEntry(groupName, entryName))
        return entry->valueInt(defaultValue);

    return defaultValue;
}

float ValueFloat(StringView groupName, StringView entryName, float defaultValue/* = 0*/)
{
    if (auto entry = FindEntry(groupName, entryName))
        return entry->valueInt(defaultValue);

    return defaultValue;
}

bool ValueBool(StringView groupName, StringView entryName, bool defaultValue/*= 0*/)
{
    if (auto entry = FindEntry(groupName, entryName))
        return entry->valueBool(defaultValue);

    return defaultValue;
}

void Write(StringView groupName, StringView entryName, StringView value)
{
    if (groupName && entryName)
    {
        auto& data = MakeEntry(groupName, entryName);
        data.value(StringBuf(value));
    }
}

void WriteInt(StringView groupName, StringView entryName, bool value)
{
    if (groupName && entryName)
    {
        auto& data = MakeEntry(groupName, entryName);
        data.value(TempString("{}", value));
    }
}

void WriteBool(StringView groupName, StringView entryName, bool value)
{
    if (groupName && entryName)
    {
        auto& data = MakeEntry(groupName, entryName);
        data.value(value ? "true" : "false");
    }
}

//--

END_BOOMER_NAMESPACE(base::config)