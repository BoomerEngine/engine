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

BEGIN_BOOMER_NAMESPACE()

//--

namespace prv
{

    class ConfigStorageData : public ISingleton
    {
        DECLARE_SINGLETON(ConfigStorageData);

    public:
        ConfigStorageData()
        {
            m_storage = new ConfigStorage;
        }

        virtual void deinit() override
        {
            delete m_storage;
        }

        ConfigStorage* m_storage;
    };
} // prv

//--

ConfigStorage& ConfigRawStorageData()
{
    return *prv::ConfigStorageData::GetInstance().m_storage;
}

//--

ConfigGroup& ConfigMakeGroup(StringView name)
{
    return prv::ConfigStorageData::GetInstance().m_storage->group(name);
}

const ConfigGroup* ConfigFindGroup(StringView name)
{
    return prv::ConfigStorageData::GetInstance().m_storage->findGroup(name);
}

Array<const ConfigGroup*> ConfigFindAllGroups(StringView groupNameSubString)
{
    return prv::ConfigStorageData::GetInstance().m_storage->findAllGroups(groupNameSubString);
}

ConfigEntry& ConfigMakeEntry(StringView groupName, StringView entryName)
{
    return ConfigMakeGroup(groupName).entry(entryName);
}

const ConfigEntry* ConfigFindEntry(StringView groupName, StringView entryName)
{
    if (auto group = ConfigFindGroup(groupName))
        return group->findEntry(entryName);

    return nullptr;
}

const Array<StringBuf> ConfigValues(StringView groupName, StringView entryName)
{
    if (auto entry = ConfigFindEntry(groupName, entryName))
        return entry->values();

    return Array<StringBuf>();
}

StringBuf ConfigValue(StringView groupName, StringView entryName, const StringBuf& defaultValue)
{
    if (auto entry = ConfigFindEntry(groupName, entryName))
        return entry->value();

    return defaultValue;
}

int ConfigValueInt(StringView groupName, StringView entryName, int defaultValue/* = 0*/)
{
    if (auto entry = ConfigFindEntry(groupName, entryName))
        return entry->valueInt(defaultValue);

    return defaultValue;
}

float ConfigValueFloat(StringView groupName, StringView entryName, float defaultValue/* = 0*/)
{
    if (auto entry = ConfigFindEntry(groupName, entryName))
        return entry->valueInt(defaultValue);

    return defaultValue;
}

bool ConfigValueBool(StringView groupName, StringView entryName, bool defaultValue/*= 0*/)
{
    if (auto entry = ConfigFindEntry(groupName, entryName))
        return entry->valueBool(defaultValue);

    return defaultValue;
}

void ConfigWrite(StringView groupName, StringView entryName, StringView value)
{
    if (groupName && entryName)
    {
        auto& data = ConfigMakeEntry(groupName, entryName);
        data.value(StringBuf(value));
    }
}

void ConfigWriteInt(StringView groupName, StringView entryName, bool value)
{
    if (groupName && entryName)
    {
        auto& data = ConfigMakeEntry(groupName, entryName);
        data.value(TempString("{}", value));
    }
}

void ConfigWriteBool(StringView groupName, StringView entryName, bool value)
{
    if (groupName && entryName)
    {
        auto& data = ConfigMakeEntry(groupName, entryName);
        data.value(value ? "true" : "false");
    }
}

//--

END_BOOMER_NAMESPACE()
