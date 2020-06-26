/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

namespace base
{
    namespace config
    {

        ///----

        /// config system, global key-value pairs used to pull configuation data from outside world without the resources
        class BASE_CONFIG_API System : public ISingleton
        {
            DECLARE_SINGLETON(System);

        public:
            System();

            //--

            // get configuration storage
            INLINE Storage& storage() { return *m_storage; }
            INLINE const Storage& storage() const { return *m_storage; }

            //--

            // get config group, creates an empty new one if not found
            Group& group(StringID name);

            // find config group, returns NULL if not found
            const Group* findGroup(StringID name) const;

            // get config entry, creates an empty new one if not found
            Entry& entry(StringID groupName, StringID entryName);

            // find config entry, return NULL if not found
            const Entry* findEntry(StringID groupName, StringID entryName) const;

            // find all groups starting with given start string
            Array<const Group*> findAllGroups(StringView<char> groupNameSubString) const;

            //--

            /// get all values, NOTE: slow as fuck but needed since we want to make the config entry thread safe
            const Array<StringBuf> values(StringID groupName, StringID entryName) const;

            /// get value (last value from a list or empty string)
            StringBuf value(StringID groupName, StringID entryName, const StringBuf& defaultValue = StringBuf::EMPTY()) const;

            /// read as integer, returns default value if not parsed correctly
            int valueInt(StringID groupName, StringID entryName, int defaultValue = 0) const;

            /// read as a float, returns default value if not parsed correctly
            float valueFloat(StringID groupName, StringID entryName, float defaultValue = 0) const;

            /// read as a boolean, returns default value if not parsed correctly
            bool valueBool(StringID groupName, StringID entryName, bool defaultValue = 0) const;

            //--

        private:
            Storage* m_storage;

            virtual void deinit() override;
        };

    } // config
} // base

typedef base::config::System Config;