/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#include "build.h"
#include "uiStyleParameters.h"
#include "uiStyleValue.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamTextReader.h"

namespace ui
{
    namespace style
    {

#if 0
        //----

        
        namespace helper
        {
            static const ParamIndex INVALID_PARAM_INDEX = (ParamIndex)~0;

            class ParamRegistry : public base::ISingleton
            {
                DECLARE_SINGLETON(ParamRegistry);

            public:
                ParamRegistry()
                {
                    //m_nameMap.reserve(100);
                    m_entries.reserve(100);
                }

                /// register a parameter
                void registerParam(const char* groupName, const char* name, const TParamValueType& valueType)
                {
                    base::StringID paramName(name);

                    ParamIndex paramIndex = 0;
                    if (m_nameMap.find(paramName, paramIndex))
                    {
                        TRACE_ERROR("Style param '{}' is already registered", name);
                        return;
                    }

                    paramIndex = range_cast<ParamIndex>(m_entries.size());
                    m_entries.emplaceBack(Entry{ paramName, valueType });
                    m_nameMap.set(paramName, paramIndex);

                    // add to group
                    if (groupName && *groupName)
                    {
                        auto& group = m_groupMap[base::StringID(groupName)];
                        group.m_params.pushBack(paramIndex);
                    }
                }

                /// locate parameter index, unknown parameters assert (for safety)
                ParamIndex findParamIndex(const base::StringID paramName) const
                {
                    ParamIndex paramIndex = 0;
                    if (m_nameMap.find(paramName, paramIndex))
                        return paramIndex;

                    return INVALID_PARAM_INDEX;
                }

                /// get all parameters in given parameter group
                void findParamsInGroup(const base::StringID groupName, base::Array<ParamIndex>& outParamIndices)
                {
                    const auto* group = m_groupMap.find(groupName);
                    if (nullptr != group)
                        outParamIndices = group->m_params;
                }

                /// get value type for parameter with given index
                base::ClassType valueType(ParamIndex index)
                {
                    auto& entry = m_entries[index];
                    if (entry.m_type == nullptr)
                        entry.m_type = entry.m_typeFunc();

                    return entry.m_type;
                }

                /// get name for parameter with given index
                base::StringID paramName(ParamIndex index) const
                {
                    return m_entries[index].m_name;
                }

                /// get default value for given value type
                const Value& defaultValue(base::ClassType valueClass)
                {
                    Value* value = nullptr;
                    if (!m_defaultValues.find(valueClass, value))
                    {
                        value = valueClass->createPointer<Value>();
                        m_defaultValues.set(valueClass, value);
                    }
                    return *value;
                }

                ///---

            private:
                struct Entry
                {
                    base::StringID m_name;
                    TParamValueType m_typeFunc;
                    base::ClassType m_type;
                };

                struct Group
                {
                    base::StringID m_name;
                    base::Array<ParamIndex> m_params;
                };

                typedef base::Array<Entry> TEntries;
                TEntries m_entries;

                typedef base::HashMap<base::StringID, ParamIndex> TNameMap;
                TNameMap m_nameMap;

                typedef base::HashMap<base::StringID, Group> TGroupMap;
                TGroupMap m_groupMap;

                typedef base::HashMap<base::ClassType, Value*> TDefaultValuesMap;
                TDefaultValuesMap m_defaultValues;

                virtual void deinit()
                {
                    m_entries.clear();
                    m_nameMap.clear();
                    m_groupMap.clear();
                    m_defaultValues.clearPtr();
                }
            };

        } // helper

        //----

        ParamGroup::ParamGroup(const char* name)
            : m_name(name)
        {}

        //----

        ParamID::ParamID()
            : m_index(0)
            , m_valueType(nullptr)
        {}

        ParamID::ParamID(const base::StringID name, ParamIndex index, base::ClassType valueType)
            : m_name(name)
            , m_index(index)
            , m_valueType(valueType)
        {}

        void ParamID::RegisterParam(const char* groupName, const char* name, const TParamValueType& valueType)
        {
            helper::ParamRegistry::GetInstance().registerParam(groupName, name, valueType);
        }

        ParamID ParamID::FindParameterByName(const base::StringID paramName)
        {
            auto index = helper::ParamRegistry::GetInstance().findParamIndex(paramName);
            if (index == helper::INVALID_PARAM_INDEX)
                return ParamID();

            auto paramType = helper::ParamRegistry::GetInstance().valueType(index);
            return ParamID(paramName, index, paramType);
        }

        void ParamID::FindParametersInGroup(const base::StringID groupName, base::Array<ParamID>& outParams)
        {
            base::Array<ParamIndex> paramIndices;
            helper::ParamRegistry::GetInstance().findParamsInGroup(groupName, paramIndices);

            outParams.reserve(paramIndices.size());
            for (const auto index : paramIndices)
            {
                auto paramName = helper::ParamRegistry::GetInstance().paramName(index);
                auto paramType = helper::ParamRegistry::GetInstance().valueType(index);
                outParams.pushBack(ParamID(paramName, index, paramType));
            }
        }

        bool ParamID::writeBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream) const
        {
            stream.writeName(m_name);
            return true;
        }

        bool ParamID::writeText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream) const
        {
            stream.writeValue(m_name.view());
            return true;
        }

        bool ParamID::readBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream)
        {
            base::StringID name = stream.readName();
            auto param = ParamID::FindParameterByName(name);
            if (!param.valid())
            {
                TRACE_ERROR("Unable to find parameter '{}' used in the style library", name);
                return false;
            }

            *this = param;
            return true;
        }

        bool ParamID::readText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextReader& stream)
        {
            base::StringView<char> text;
            if (!stream.readValue(text))
            {
                TRACE_ERROR("Unable to load parameter name");
                return false;
            }

            auto param = ParamID::FindParameterByName(base::StringID(text));
            if (!param.valid())
            {
                TRACE_ERROR("Unable to find parameter '{}' used in the style library", text);
                return false;
            }

            *this = param;
            return true;
        }

        void ParamID::calcHash(base::CRC64& crc) const
        {
            crc << m_name;
        }

        //----

        ParamTable::ParamTable(uint64_t compoundStyleHash, const StyleLibraryPtr& library)
            : m_compoundStyleHash(compoundStyleHash)
            , m_library(library)
        {
        }

        ParamTable::ParamTable(const ParamTable& other)
            : m_params(other.m_params)
            , m_compoundStyleHash(other.m_compoundStyleHash)
        {}

        ParamTable::ParamTable(ParamTable&& other)
            : m_params(std::move(other.m_params))
            , m_compoundStyleHash(other.m_compoundStyleHash)
        {
            other.m_compoundStyleHash = 0;
        }

        ParamTable& ParamTable::operator=(ParamTable&& other)
        {
            if (this != &other)
            {
                m_params = std::move(other.m_params);
                m_compoundStyleHash = other.m_compoundStyleHash;
                other.m_compoundStyleHash = 0;
            }

            return *this;
        }

        ParamTable& ParamTable::operator=(const ParamTable& other)
        {
            if (this != &other)
            {
                m_params = other.m_params;
                m_compoundStyleHash = other.m_compoundStyleHash;
            }

            return *this;
        }

        void ParamTable::set(const ParamID& param, const Value* valuePtr)
        {
            ASSERT(param.valid());
            ASSERT(valuePtr != nullptr);

            auto index = param.paramIndex();
            ASSERT(valuePtr->is(param.valueType()));

            m_params.set(index, valuePtr); // if the valid is already there it's not set, this works because we are visiting the selectors bottom-up
        }

        const Value& ParamTable::get(const ParamID& param, base::ClassType paramClass) const
        {
            ASSERT(paramClass && paramClass->is<Value>());

            const auto* value = m_params.findSafe(param.paramIndex(), nullptr);
            if (nullptr != value)
            {
                ASSERT(value->is(paramClass));
                return *value;
            }

            return helper::ParamRegistry::GetInstance().defaultValue(paramClass);
        }

        const Value* ParamTable::ptr(const ParamID& param, base::ClassType paramClass) const
        {
            ASSERT(paramClass && paramClass->is<Value>());

            const auto* value = m_params.findSafe(param.paramIndex(), nullptr);
            if (nullptr != value)
            {
                ASSERT(value->is(paramClass));
                return value;
            }

            return nullptr;
        }

        //--

        class ParamTableRegistry : public base::ISingleton
        {
            DECLARE_SINGLETON(ParamTableRegistry);

        public:
            ParamTableRegistry()
            {
                m_nullTable = base::CreateSharedPtr<ParamTable>(0, nullptr);
            }

            INLINE const ParamTablePtr& nullTable()
            {
                return m_nullTable;
            }

        private:
            ParamTablePtr m_nullTable;

            virtual void deinit() override
            {
                m_nullTable.reset();
            }
        };


        ParamTablePtr ParamTable::NullTable()
        {
            return ParamTableRegistry::GetInstance().nullTable();
        }

        //--
#endif

    } // style
} // ui
