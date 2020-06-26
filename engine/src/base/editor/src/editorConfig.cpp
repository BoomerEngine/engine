/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "editorService.h"
#include "editorConfig.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/utils.h"
#include "base/config/include/configStorage.h"
#include "base/config/include/configEntry.h"
#include "base/config/include/configGroup.h"

namespace ed
{
    //--

    INLINE static bool ValidConfigChar(char ch)
    {
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == '_') return true;
        return false;
    }

    static StringBuf ConformConfigName(const StringView<char> view)
    {
        StringBuilder ret;

        // do we have any chars that are invalid ?
        const char* ptr = view.data();
        const char* ptrEnd = view.data() + view.length();
        while (ptr < ptrEnd)
        {
            if (!ValidConfigChar(*ptr))
                break;
            ++ptr;
        }

        // use the valid part
        ret.append(view.data(), ptr - view.data());

        // if we still have something unparsed make a hash out of it
        if (ptr < ptrEnd)
        {
            const auto remainingTxt = StringView<char>(ptr, ptrEnd);
            ret.appendf("_{}", Hex(remainingTxt.calcCRC64()));
        }

        return ret.toString();
    }

    //--

    ConfigGroup::ConfigGroup()
    {
    }

    ConfigGroup::ConfigGroup(const ConfigGroup& parent, StringView<char> name)
        : m_root(parent.m_root)
    {
        auto safeName = ConformConfigName(name);

        if (parent.m_path)
            m_path = TempString("{}.{}", parent.m_path, name);
        else if (name)
            m_path = StringBuf(name);
    }

    bool ConfigGroup::readRaw(StringView<char> groupName, StringView<char> key, Type valueType, void* value) const
    {
        if (m_root)
            return m_root->readRaw(groupName, key, valueType, value);
        else
            return false;
    }

    void ConfigGroup::writeRaw(StringView<char> groupName, StringView<char> key, Type valueType, const void* value)
    {
        if (m_root)
            m_root->writeRaw(groupName, key, valueType, value);
    }

    ConfigGroup ConfigGroup::operator[](StringView<char> child) const
    {
        return ConfigGroup(*this, child);
    }

    //--

    ConfigRoot::ConfigRoot()
    {
        m_storage.create();
        m_root = this;
    }

    ConfigRoot::~ConfigRoot()
    {}

    bool ConfigRoot::load(const io::AbsolutePath& filePath)
    {
        StringBuf txt;
        if (io::LoadFileToString(filePath, txt))
            if (config::Storage::Load(txt, *m_storage))
                return true;

        return false;
    }

    bool ConfigRoot::save(const io::AbsolutePath& filePath)
    {
        // save to text
        StringBuilder txt;
        {
            config::Storage deltaStorage;
            config::Storage::Save(txt, *m_storage, deltaStorage);
        }

        // save the file
        return io::SaveFileFromString(filePath, txt.view());
    }

    bool ConfigRoot::readRaw(StringView<char> groupName, StringView<char> key, Type valueType, void* value) const
    {
        DEBUG_CHECK_EX(valueType != nullptr, "No value type specified");
        DEBUG_CHECK_EX(value != nullptr, "Invalid value pointer");

        if (!groupName)
            groupName = "Editor";

        if (valueType && value)
        {
            if (auto group = m_storage->findGroup(StringID::Find(groupName)))
            {
                if (auto entry = group->findEntry(StringID::Find(key)))
                {
                    return ConfigPropertyBase::LoadFromEntry(valueType, value, nullptr, *entry);
                }
            }
        }

        return false;
    }

    void ConfigRoot::writeRaw(StringView<char> groupName, StringView<char> key, Type valueType, const void* value)
    {
        DEBUG_CHECK_EX(valueType != nullptr, "No value type specified");
        DEBUG_CHECK_EX(value != nullptr, "Invalid value pointer");

        if (valueType && value)
            
            if (!groupName)
                groupName = "Editor"; 
        {
            auto& group = m_storage->group(StringID(groupName));
            auto& entry = group.entry(StringID(key));

            ConfigPropertyBase::SaveToEntry(valueType, value, nullptr, entry);
        }
    }

    //--

} // editor
