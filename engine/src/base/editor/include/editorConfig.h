/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "base/reflection/include/reflectionTypeName.h"

namespace ed
{

    ///--

    /// configuration group
    class BASE_EDITOR_API ConfigGroup
    {
    public:
        ConfigGroup();
        ConfigGroup(const ConfigGroup& parent, StringView<char> name);
        ConfigGroup(const ConfigGroup& other) = default;
        ConfigGroup(ConfigGroup&& other) = default;
        ConfigGroup& operator=(const ConfigGroup& other) = default;
        ConfigGroup& operator=(ConfigGroup&& other) = default;

        // get path, usually a string like "Editor.Settings"
        INLINE const StringBuf& path() const { return m_path; }

        //--

        // read config value from current group or return the default
        template< typename T >
        INLINE T readOrDefault(StringView<char> key, T defaultValue = T()) const
        {
            T ret = defaultValue;
            if (readRaw(m_path, key, reflection::GetTypeObject<T>(), &ret))
                return ret;
            return defaultValue;
        }

        // read config value from current group or return the default
        template< typename T >
        INLINE bool read(StringView<char> key, T& ret) const
        {
            return readRaw(m_path, key, reflection::GetTypeObject<T>(), &ret);
        }

        // write config value to current group 
        template< typename T >
        INLINE void write(StringView<char> key, const T& data)
        {
            writeRaw(m_path, key, reflection::GetTypeObject<T>(), &data);
        }

        //--

        // get child group
        ConfigGroup operator[](StringView<char> child) const;

    protected:
        virtual bool readRaw(StringView<char> groupName, StringView<char> key, Type valueType, void* value) const;
        virtual void writeRaw(StringView<char> groupName, StringView<char> key, Type valueType, const void* value);

        ConfigGroup* m_root = nullptr;
        StringBuf m_path;
    };

    ///--

    /// root configuration item, saves the stuff to a config entry
    class BASE_EDITOR_API ConfigRoot : public ConfigGroup
    {
    public:
        ConfigRoot();
        ~ConfigRoot();

        //--

        // get storage
        INLINE config::Storage& storage() { return *m_storage; }
        INLINE const config::Storage& storage() const { return *m_storage; }

        //--

        // load values from file
        bool load(const io::AbsolutePath& filePath);

        // store values to file
        bool save(const io::AbsolutePath& filePath);

        //--

    private:
        Mutex m_storageLock;
        UniquePtr<config::Storage> m_storage;

        virtual bool readRaw(StringView<char> groupName, StringView<char> key, Type valueType, void* value) const override final;
        virtual void writeRaw(StringView<char> groupName, StringView<char> key, Type valueType, const void* value) override final;
    };

    //--

} // ed

