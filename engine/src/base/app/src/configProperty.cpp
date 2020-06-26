/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "configProperty.h"
#include "base/config/include/configStorage.h"
#include "base/config/include/configGroup.h"
#include "base/config/include/configEntry.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/inplaceArray.h"
#include "base/object/include/streamTextReader.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/rttiArrayType.h"
#include "base/reflection/include/variant.h"

namespace base
{
    //---

    namespace helper
    {
        namespace helper
        {
            static bool ShouldEscapeString(StringView<char> txt)
            {
                if (txt.empty())
                    return true; // escape empty strings

                for (auto ch  : txt)
                {
                    if (ch >= 'A' && ch <= 'Z') continue;
                    if (ch >= 'a' && ch <= 'a') continue;
                    if (ch >= '0' && ch <= '9') continue;
                    if (ch == '.' || ch == '_') continue;
                    return true;
                }

                return false;
            }

            static void WriteEscapedString(IFormatStream& builder, StringView<char> txt)
            {
                builder.append("\"");

                for (auto ch  : txt)
                {
                    if (ch == '\n')
                    {
                        builder.append("\\n");
                    }
                    else if (ch == '\r')
                    {
                        builder.append("\\r");
                    }
                    else if (ch == '\t')
                    {
                        builder.append("\\t");
                    }
                    else if (ch == '\b')
                    {
                        builder.append("\\b");
                    }
                    else if (ch == '\"')
                    {
                        builder.append("\\\"");
                    }
                    else if (ch < 32 || ch > 127)
                    {
                        builder.append("\\x%04u", ch);
                    }
                    else
                    {
                        const char str[] = {ch, 0};
                        builder.append(str);
                    }
                }

                builder.append("\"");
            }

        } // helper

        //--

        class ConfigPropertyRegistry : public ISingleton
        {
            DECLARE_SINGLETON(ConfigPropertyRegistry);

        public:
            void registerProperty(ConfigPropertyBase* prop)
            {
                auto lock  = CreateLock(m_lock);
                m_list.insert(prop);
            }

            void unegisterProperty(ConfigPropertyBase* prop)
            {
                auto lock  = CreateLock(m_lock);
                m_list.remove(prop);
            }

            void all(Array<ConfigPropertyBase*> &outProperties)
            {
                auto lock  = CreateLock(m_lock);
                outProperties.reserve(m_list.size());

                for (auto prop  : m_list)
                    outProperties.pushBack(prop);
            }

            void run(const std::function<void(ConfigPropertyBase*)>& func)
            {
                auto lock  = CreateLock(m_lock);

                for (auto prop  : m_list)
                    func(prop);
            }

        private:
            virtual void deinit() override
            {
                m_list.clear();
            }

            HashSet<ConfigPropertyBase*> m_list;
            SpinLock m_lock;
        };

    } // helper

    //---

    ConfigPropertyBase::ConfigPropertyBase(StringID groupName, StringID name, ConfigPropertyFlags flags)
        : m_group(groupName)
        , m_name(name)
        , m_flags(flags)
    {}

    ConfigPropertyBase::~ConfigPropertyBase()
    {
        helper::ConfigPropertyRegistry::GetInstance().unegisterProperty(this);
    }

    void ConfigPropertyBase::registerInList()
    {
        helper::ConfigPropertyRegistry::GetInstance().registerProperty(this);
    }

    void ConfigPropertyBase::print(IFormatStream& txt) const
    {
        if (auto type  = this->type())
        {
            // arrays are written as multiple entries
            if (type.isArray())
            {
                auto arrayType  = static_cast<const rtti::IArrayType *>(type.ptr());
                auto size  = arrayType->arraySize(data());

                for (uint32_t i = 0; i < size; ++i)
                {
                    auto elementData  = arrayType->arrayElementData(data(), i);

                    txt.append(name().c_str());

                    if (i > 0)
                        txt.append("+=");
                    else
                        txt.append("=");

                    type->printToText(txt, elementData);

                    txt.append("\n");
                }
            }
            else
            {
                txt.append(name().c_str());
                txt.append("=");
                type->printToText(txt, data());
                txt.append("\n");
            }
        }
    }

    bool ConfigPropertyBase::pullValueFromConfig()
    {
        if (auto type  = this->type())
        {
            if (auto entry  = Config::GetInstance().findEntry(m_group, m_name))
            {
                 if (!LoadFromEntry(type, data(), defaultData(), *entry))
                 {
                     TRACE_ERROR("Failed to load config element {}.{}", group(), name());
                     return false;
                 }

                 return true;
            }
        }

        return false;
    }

    void ConfigPropertyBase::SaveToEntry(base::Type type, const void* data, const void* defaultData, config::Entry& entry)
    {
        DEBUG_CHECK(type);
        DEBUG_CHECK(data);

        // remove any existing value(s)
        entry.clear();

        // setup serialization
        StringBuilder txt;

        // arrays are written as multiple entries
        if (type->metaType() == rtti::MetaType::Array)
        {
            auto arrayType  = static_cast<const rtti::IArrayType *>(type.ptr());
            auto size  = arrayType->arraySize(data);

            for (uint32_t i = 0; i < size; ++i)
            {
                auto elementData = arrayType->arrayElementData(data, i);
                auto elementDefaultData = defaultData ? arrayType->arrayElementData(defaultData, i) : nullptr;

                txt.clear();
                arrayType->innerType()->printToText(txt, elementData, rtti::PrintToFlag_TextSerializaitonStructElement); // force quotes
                entry.appendValue(txt.toString());
            }
        }
        else
        {
            type->printToText(txt, data, rtti::PrintToFlag_TextSerializaitonStructElement); // force quotes
            entry.appendValue(txt.toString());
        }
    }

    bool ConfigPropertyBase::LoadFromEntry(base::Type type, void* data, const void* defaultData, const config::Entry& entry)
    {
        DEBUG_CHECK(type);
        DEBUG_CHECK(data);

        Variant newValue;
        newValue.init(type, defaultData);

        if (type->metaType() == rtti::MetaType::Array)
        {
            auto arrayType = static_cast<const rtti::IArrayType *>(type.ptr());
            auto values = entry.values(); // TODO: monitor performance of this crap...

            arrayType->clearArrayElements(newValue.data());

            for (auto &value : values)
            {
                auto index = arrayType->arraySize(newValue.data());
                if (!arrayType->createArrayElement(newValue.data(), index))
                    return false;

                auto elementData = arrayType->arrayElementData(newValue.data(), index);
                if (!arrayType->innerType()->parseFromString(value, elementData))
                    return false;
            }
        }
        else
        {
            auto value = entry.value();

            // parse the value
            if (!type->parseFromString(value, newValue.data()))
                return false;

            /*// print update
            if (!type->compare(data, newValue.data()))
            {
                StringBuilder old, cur;

                helper::ConfigTextWriter oldWriter(old), curWriter(cur);
                type->writeText(context, oldWriter, data, nullptr);
                type->writeText(context, curWriter, newValue.data(), nullptr);
            }*/
        }

        // update value
        type->copy(data, newValue.data());
        return true;
    }

    void ConfigPropertyBase::pushValueToConfig() const
    {
        if (auto type = this->type())
        {
            if (!type->compare(data(), defaultData()))
            {
                // get the configuration entry
                auto &entry = Config::GetInstance().entry(m_group, m_name);
                SaveToEntry(type, data(), defaultData(), entry);
            }
        }
    }

    void ConfigPropertyBase::GetAllProperties(base::Array<ConfigPropertyBase*>& outProperties)
    {
        helper::ConfigPropertyRegistry::GetInstance().all(outProperties);
    }

    void ConfigPropertyBase::PullAll()
    {
        helper::ConfigPropertyRegistry::GetInstance().run([](ConfigPropertyBase* prop){
            prop->pullValueFromConfig();
        });
    }

    void ConfigPropertyBase::PrintAll(IFormatStream& txt)
    {
        Array<ConfigPropertyBase*> allValues;
        helper::ConfigPropertyRegistry::GetInstance().all(allValues);

        std::sort(allValues.begin(), allValues.end(), [](const ConfigPropertyBase* a, const ConfigPropertyBase* b)
        {
            if (a->group() != b->group())
                return a->group() < b->group();

            return a->name() < b->name();
        });

        StringID groupName;
        for (auto prop  : allValues)
        {
            if (groupName != prop->group())
            {
                if (groupName)
                    txt.append("\n");

                groupName = prop->group();
                txt.appendf("[{}]\n", groupName);
            }

            prop->print(txt);
        }
    }

    //---

} // base
