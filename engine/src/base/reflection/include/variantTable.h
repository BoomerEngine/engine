/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: variant #]
***/

#pragma once

#include "reflectionMacros.h"
#include "base/containers/include/hashMap.h"
#include "variant.h"

namespace base
{
    /// entry in the variant table
    struct VariantTableEntry
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(VariantTableEntry);

    public:
        StringID name;
        Variant data;
    };

    /// a key/value parameter table
    /// allows to store set of named values without limiting ourselves to one type
    /// use the reflection::Parameter table for the extended version (with template operators)
    /// NOTE: the parameter table is a custom type in RTTI this allows the table to store another table
    /// NOTE: the table supports deep copy but it's very slow, it's better to hold on to it by pointer
    class BASE_REFLECTION_API VariantTable
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(VariantTable);

    public:
        VariantTable();

        //--

        // empty ?
        INLINE bool empty() const { return m_parameters.empty(); }

        // get raw list of parameters
        INLINE const Array<VariantTableEntry>& parameters() const { return m_parameters; }

        //--

        // remove all values from the table
        void clear();

        // check if the table contains a given entry
        bool contains(StringID name) const;

        // remove entry from the table, returns true if item was removed
        // NOTE: after some amount of removed entries the table should be compacted
        bool remove(StringID name);

        //-

        // set value of entry in the table
        void setVariant(StringID name, const Variant& value);
        void setVariant(StringID name, Variant&& value); // for set("key"_id, CreateVariant<int>(42)) case

        // get value for given entry
        const Variant& getVariant(StringID name) const;

        // find value for given entry, returns nullptr if not found
        const Variant* findVariant(StringID name) const;

        //--

        // apply entries from other table
        void apply(const VariantTable& otherTable);

        //---

        // print to stream as a "key=value" pair
        void print(IFormatStream& stream, bool oneline = false) const;

        //--

        // find value in table
        template< typename T >
        INLINE T getValueOrDefault(StringID name, const T& defaultValue = T()) const
        {
            if (const auto* value = findVariant(name))
                return value->getOrDefault(defaultValue);
            return defaultValue;
        }

        // get data from table
        template< typename T >
        INLINE bool getValue(StringID name, T& outValue) const
        {
            if (const auto* value = findVariant(name))
                return value->get(outValue);
            return false;
        }

        // set value in table
        template< typename T >
        INLINE void setValue(StringID name, const T& value)
        {
            DEBUG_CHECK_EX(name, "Empty name");
            static_assert(!std::is_same < std::remove_cv<T>::type, Variant >::value, "Trying to add a variant inside a variant");
            setVariant(name, CreateVariant<T>(value));
        }

        // calculate hash of the stored data
        void calcCRC64(CRC64& crc) const;

    private:
        base::Array<VariantTableEntry> m_parameters;
    };

} // base

