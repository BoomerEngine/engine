/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "localService.h"

#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE()

//----

/// configuration properties flags
enum class ConfigPropertyFlag : uint8_t
{
    ReadOnly = 1, // value cannot be changed from the console
    Developer = 2, // value is not visible in normal listings in console
};

typedef DirectFlags<ConfigPropertyFlag> ConfigPropertyFlags;

//----

// configuration property
class CORE_APP_API ConfigPropertyBase : public NoCopy
{
public:
    ConfigPropertyBase(StringView groupName, StringView name, ConfigPropertyFlags flags);
    virtual ~ConfigPropertyBase();

    /// get the group name (config group)
    INLINE const StringBuf& group() const { return m_group; }

    /// get the property name
    INLINE const StringBuf& name() const { return m_name; }

    /// is this config value modifiable only from console?
    INLINE bool isReadOnly() const { return m_flags.test(ConfigPropertyFlag::ReadOnly); }

    /// is this config value hidden from normal listings
    INLINE bool isDeveloper() const { return m_flags.test(ConfigPropertyFlag::Developer); }

    ///--

    /// get type of the property
    virtual Type type() const = 0;

    /// get store data
    virtual void* data() = 0;
    virtual const void* data() const = 0;

    /// get default data
    virtual const void* defaultData() const = 0;

    ///--

    /// get all registered config properties
    static void GetAllProperties(Array<ConfigPropertyBase*>& outProperties);

    /// save current values to text output (for debug)
    static void PrintAll(IFormatStream& txt);

    /// refresh all properties from config
    static void PullAll();

    ///--

    /// save value to storage entry
    static void SaveToEntry(Type dataType, const void* data, const void* defaultData, config::Entry& outEntry);

    /// load value from storage entry
    static bool LoadFromEntry(Type dataType, void* data, const void* defaultData, const config::Entry& entry);

    ///--

    /// refresh property value from entry
    static void RefreshPropertyValue(StringView group, StringView name);

protected:
    StringBuf m_group;
    StringBuf m_name;
    ConfigPropertyFlags m_flags;

    void registerInList();
    void pushValueToConfig() const;
    bool pullValueFromConfig();
    void print(IFormatStream& txt) const;
};

//----

// typed configuration property
template< typename T >
class ConfigProperty : public ConfigPropertyBase
{
public:
    INLINE ConfigProperty(StringView groupName, StringView name, const T& value, ConfigPropertyFlags flags = ConfigPropertyFlags())
        : ConfigPropertyBase(groupName, name, flags)
        , m_defaultValue(value)
        , m_value(value)
    {
        registerInList();
    }

    /// get value of the property
    INLINE const T& get() const
    {
        return m_value;
    }

    /// get value of the property
    INLINE T& get()
    {
        return m_value;
    }

    /// set value of the property
    INLINE void set(const T& newValue)
    {
        if (m_value != newValue)
        {
            m_value = newValue;
            pushValueToConfig();
        }
    }

protected:
    T m_value;
    T m_defaultValue;

    virtual void* data() override final { return &m_value; };
    virtual const void* data() const override final { return &m_value; };
    virtual const void* defaultData() const override final { return &m_defaultValue; };

    virtual Type type() const override final
    {
        static Type theResolvedType = nullptr;
        if (!theResolvedType)
            theResolvedType = GetTypeObject<T>();
        return theResolvedType;
    }
};

//----

END_BOOMER_NAMESPACE()
