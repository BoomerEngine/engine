/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "uiStyleLibrary.h"

BEGIN_BOOMER_NAMESPACE(ui)

//------

/// configuration writer interface
class BASE_UI_API IConfigDataInterface : public base::NoCopy
{
public:
    virtual ~IConfigDataInterface();

    // write data to config
    virtual void writeRaw(base::StringView path, base::StringView key, base::Type type, const void* data) = 0;

    // read data from config
    virtual bool readRaw(base::StringView path, base::StringView key, base::Type type, void* data) const = 0;
};

//------

/// helper for config blocks
class BASE_UI_API ConfigBlock
{
public:
    ConfigBlock(IConfigDataInterface* data, base::StringView rootTag);
    ConfigBlock(const ConfigBlock& other) = default;
    ConfigBlock& operator=(const ConfigBlock& other) = default;
    ~ConfigBlock();

    // read config value to current group 
    template< typename T >
    INLINE bool read(base::StringView key, T& data) const
    {
        return m_data->readRaw(m_path, key, base::reflection::GetTypeObject<T>(), &data);
    }

    // read config value from current group or return the default
    template< typename T >
    INLINE T readOrDefault(base::StringView key, T defaultValue = T()) const
    {
        T ret;
        if (m_data->readRaw(m_path, key, base::reflection::GetTypeObject<T>(), &ret))
            return ret;

        return defaultValue;
    }

    // write config value to current group 
    template< typename T >
    INLINE void write(base::StringView key, const T& data) const
    {
        m_data->writeRaw(m_path, key, base::reflection::GetTypeObject<T>(), &data);
    }

    //--

    // get a child block for given tag
    ConfigBlock tag(base::StringView tag) const;

    //--

private:
    base::StringBuf m_path;
    IConfigDataInterface* m_data;
};

//------

// simple, .ini based config storage
class BASE_UI_API ConfigFileStorageDataInterface : public IConfigDataInterface
{
public:
    ConfigFileStorageDataInterface();
    ~ConfigFileStorageDataInterface();

    /// load data from file
    bool loadFromFile(base::StringView path);

    /// save data to file
    bool saveToFile(base::StringView path) const;

    //--

    // IConfigDataInterface
    virtual void writeRaw(base::StringView path, base::StringView key, base::Type type, const void* data) override;
    virtual bool readRaw(base::StringView path, base::StringView key, base::Type type, void* data) const override;

    //--

private:
    struct Value
    {
        RTTI_DECLARE_POOL(POOL_CONFIG);

    public:
        base::StringBuf key;
        base::Variant value;

        Value();
        ~Value();

        bool read(base::Type type, void* data) const;
        bool write(base::Type type, const void* data);

        void save(base::xml::IDocument& doc, base::xml::NodeID nodeId) const;
        bool load(const base::xml::IDocument& doc, base::xml::NodeID nodeId);
    };

    struct Group
    {
        RTTI_DECLARE_POOL(POOL_CONFIG);

    public:
        base::StringBuf name;
        base::Array<Group*> children;
        base::Array<Value*> values;

        Group();
        ~Group();

        Value* findOrCreate(base::StringView key);
        const Value* findExisting(base::StringView key) const;

        void save(base::xml::IDocument& doc, base::xml::NodeID nodeId) const;
        bool load(const base::xml::IDocument& doc, base::xml::NodeID nodeId);
    };

    Group* findOrCreateEntry(base::StringView path);
    const Group* findExistingEntry(base::StringView path) const;

    Group* m_root = nullptr;
};

//------

END_BOOMER_NAMESPACE(ui)
