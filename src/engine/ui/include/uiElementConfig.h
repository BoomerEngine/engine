/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "uiStyleLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//------

/// configuration writer interface
class ENGINE_UI_API IConfigDataInterface : public NoCopy
{
public:
    virtual ~IConfigDataInterface();

    // write data to config
    virtual void writeRaw(StringView path, StringView key, Type type, const void* data) = 0;

    // read data from config
    virtual bool readRaw(StringView path, StringView key, Type type, void* data) const = 0;
};

//------

/// helper for config blocks
class ENGINE_UI_API ConfigBlock
{
public:
    ConfigBlock(IConfigDataInterface* data, StringView rootTag);
    ConfigBlock(const ConfigBlock& other) = default;
    ConfigBlock& operator=(const ConfigBlock& other) = default;
    ~ConfigBlock();

    // read config value to current group 
    template< typename T >
    INLINE bool read(StringView key, T& data) const
    {
        return m_data->readRaw(m_path, key, reflection::GetTypeObject<T>(), &data);
    }

    // read config value from current group or return the default
    template< typename T >
    INLINE T readOrDefault(StringView key, T defaultValue = T()) const
    {
        T ret;
        if (m_data->readRaw(m_path, key, reflection::GetTypeObject<T>(), &ret))
            return ret;

        return defaultValue;
    }

    // write config value to current group 
    template< typename T >
    INLINE void write(StringView key, const T& data) const
    {
        m_data->writeRaw(m_path, key, reflection::GetTypeObject<T>(), &data);
    }

    //--

    // get a child block for given tag
    ConfigBlock tag(StringView tag) const;

    //--

private:
    StringBuf m_path;
    IConfigDataInterface* m_data;
};

//------

// simple, .ini based config storage
class ENGINE_UI_API ConfigFileStorageDataInterface : public IConfigDataInterface
{
public:
    ConfigFileStorageDataInterface();
    ~ConfigFileStorageDataInterface();

    /// load data from file
    bool loadFromFile(StringView path);

    /// save data to file
    bool saveToFile(StringView path) const;

    //--

    // IConfigDataInterface
    virtual void writeRaw(StringView path, StringView key, Type type, const void* data) override;
    virtual bool readRaw(StringView path, StringView key, Type type, void* data) const override;

    //--

private:
    struct Value
    {
        RTTI_DECLARE_POOL(POOL_CONFIG);

    public:
        StringBuf key;
        Variant value;

        Value();
        ~Value();

        bool read(Type type, void* data) const;
        bool write(Type type, const void* data);

        void save(xml::IDocument& doc, xml::NodeID nodeId) const;
        bool load(const xml::IDocument& doc, xml::NodeID nodeId);
    };

    struct Group
    {
        RTTI_DECLARE_POOL(POOL_CONFIG);

    public:
        StringBuf name;
        Array<Group*> children;
        Array<Value*> values;

        Group();
        ~Group();

        Value* findOrCreate(StringView key);
        const Value* findExisting(StringView key) const;

        void save(xml::IDocument& doc, xml::NodeID nodeId) const;
        bool load(const xml::IDocument& doc, xml::NodeID nodeId);
    };

    Group* findOrCreateEntry(StringView path);
    const Group* findExistingEntry(StringView path) const;

    Group* m_root = nullptr;
};

//------

END_BOOMER_NAMESPACE_EX(ui)
