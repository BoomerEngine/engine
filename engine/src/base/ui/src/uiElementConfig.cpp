/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElementConfig.h"
#include "base/config/include/configStorage.h"
#include "base/config/include/configGroup.h"
#include "base/config/include/configEntry.h"
#include "base/app/include/configProperty.h"

namespace ui
{

    //---

    static bool IsValidChar(char ch)
    {
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == '_') return true;
        return false;
    }

    static bool IsValidTag(base::StringView<char> tag)
    {
        if (tag.empty())
            return false;

        for (const auto ch : tag)
            if (!IsValidChar(ch))
                return false;
        return true;
    }

    //---

    IConfigDataInterface::~IConfigDataInterface()
    {}

    //---

    ConfigBlock::ConfigBlock(IConfigDataInterface* data, base::StringView<char> rootTag)
        : m_data(data)
        , m_path(rootTag)
    {}

    ConfigBlock::~ConfigBlock()
    {}


    ConfigBlock ConfigBlock::tag(base::StringView<char> tag) const
    {
        base::StringBuilder txt;

        if (!m_path.empty())
        {
            txt << m_path;
            txt << ".";
        }

        for (const auto ch : tag)
            if (IsValidChar(ch))
                txt << ch;

        return ConfigBlock(m_data, txt.view());
    }

    ConfigBlock ConfigBlock::path(base::StringView<char> path) const
    {
        base::StringBuilder txt;

        if (!m_path.empty())
        {
            txt << m_path;
            txt << ".";
        }

        txt << "Path_";
        txt << Hex(path.calcCRC64());

        if (!path.empty())
        {
            const auto fileName = path.afterLast("/").beforeFirst(".");

            txt << "_";

            for (const auto ch : fileName)
                if (IsValidChar(ch))
                    txt << ch;
        }

        return ConfigBlock(m_data, txt.view());
    }

    //---

    ConfigFileStorageDataInterface::ConfigFileStorageDataInterface()
    {
        m_storage = MemNew(base::config::Storage);
    }

    ConfigFileStorageDataInterface::~ConfigFileStorageDataInterface()
    {
        MemDelete(m_storage);
        m_storage = nullptr;
    }

    bool ConfigFileStorageDataInterface::loadFromFile(base::StringView<char> path)
    {
        base::StringBuf content;
        if (!base::io::LoadFileToString(path, content))
            return false;

        return base::config::Storage::Load(content, *m_storage);
    }

    bool ConfigFileStorageDataInterface::saveToFile(base::StringView<char> path) const
    {
        base::StringBuilder txt;

        base::config::Storage base;
        base::config::Storage::Save(txt, *m_storage, base);

        return base::io::SaveFileFromString(path, txt.view());
    }

    void ConfigFileStorageDataInterface::writeRaw(base::StringView<char> path, base::StringView<char> key, base::Type type, const void* data)
    {
        DEBUG_CHECK_RETURN(type);
        DEBUG_CHECK_RETURN(data);
        DEBUG_CHECK_RETURN(path);
        DEBUG_CHECK_RETURN(key);

        auto& group = m_storage->group(path);
        auto& entry = group.entry(key);

        base::ConfigPropertyBase::SaveToEntry(type, data, nullptr, entry);
    }

    bool ConfigFileStorageDataInterface::readRaw(base::StringView<char> path, base::StringView<char> key, base::Type type, void* data) const
    {
        DEBUG_CHECK_RETURN_V(type, false);
        DEBUG_CHECK_RETURN_V(data, false);
        DEBUG_CHECK_RETURN_V(path, false);
        DEBUG_CHECK_RETURN_V(key, false);

        if (auto group = m_storage->findGroup(path))
        {
            if (auto entry = group->findEntry(key))
            {
                return base::ConfigPropertyBase::LoadFromEntry(type, data, nullptr, *entry);
            }
        }

        return false;
    }

    //---

} // ui
