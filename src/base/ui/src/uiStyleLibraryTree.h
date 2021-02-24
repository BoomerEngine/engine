/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "uiStyleValue.h"
#include "uiStyleSelector.h"
#include "base/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE(ui::style)

namespace prv
{

    /// base element of parsed styles, contains file name and line
    class RawBaseElement : public base::NoCopy
    {
    public:
        RawBaseElement(const base::parser::Location& location);

        /// get location in file this element was declared at
        INLINE const base::parser::Location& location() const { return m_fileLocation; }

    private:
        base::parser::Location m_fileLocation;
    };          

    /// variable
    struct RawVariable : public RawBaseElement
    {
    public:
        RawVariable(const base::parser::Location& location, const base::StringID name, const RawValue* value);

        /// get name of the property
        INLINE base::StringID name() const { return m_name; }

        /// get values of the variable
        INLINE const RawValue* value() const { return m_value; }

        //--

        // get a string representation of the value (CSS style)
        void print(base::IFormatStream& f) const;

    private:
        base::StringID m_name;
        const RawValue* m_value;
    };

    /// raw property
    struct RawProperty : public RawBaseElement
    {
    public:
        RawProperty(const base::parser::Location& location, const base::StringID name, uint32_t numValues, const RawValue** values);

        /// get name of the property
        INLINE base::StringID name() const { return m_name; }

        /// get number of values in property
        INLINE uint32_t numValues() const { return m_numValues; }

        /// get values of the property, multiple values are aggregated
        INLINE const RawValue** values() const { return m_values; }

        //--

        // get a string representation of the value (CSS style)
        void print(base::IFormatStream& f) const;

    private:
        base::StringID m_name;
        uint32_t m_numValues;
        const RawValue** m_values;
    };

    /// match list, a list of matchable tokens
    class RawMatchList
    {
    public:
        RawMatchList();

        // get entries
        typedef base::Array<base::StringID> TEntries;
        INLINE const TEntries& entries() const { return m_entries; }

        // is this list empty ?
        INLINE bool empty() const { return m_entries.empty(); }

        //--

        // add element to match list
        // NOTE: duplicates are not added
        void add(const base::StringID id);

        // add element to match list
        // NOTE: duplicates are not added
        void add(const char* txt);

        // merge match lists
        void merge(const RawMatchList& other);

    private:
        TEntries m_entries;
    };

    /// raw selector
    class RawSelector : public RawBaseElement
    {
    public:
        RawSelector(const base::parser::Location& location, SelectorCombinatorType combinator);

        /// get combinator type for selector
        INLINE SelectorCombinatorType combinator() const { return m_combinator; }

        /// get the matched types (button, panel)
        INLINE const RawMatchList& types() const { return m_types; }
        INLINE RawMatchList& types() { return m_types; }

        /// get the matched classes (.big,.toolbar)
        INLINE const RawMatchList& classes() const { return m_classes; }
        INLINE RawMatchList& classes() { return m_classes; }

        /// get the matched identifiers (#exit, #dupa)
        INLINE const RawMatchList& identifiers() const { return m_ids; }
        INLINE RawMatchList& identifiers() { return m_ids; }

        /// get the pseudo classes (:hover,:disabled)
        INLINE const RawMatchList& pseudoClasses() const { return m_pseudoClasses; }
        INLINE RawMatchList& pseudoClasses() { return m_pseudoClasses; }

        //--

        // get a text representation of the selector (CSS  style) 
        void print(base::IFormatStream& str) const;

    private:
        SelectorCombinatorType m_combinator;
        RawMatchList m_types;
        RawMatchList m_classes;
        RawMatchList m_ids;
        RawMatchList m_pseudoClasses;
    };

    /// rule - a selector chain
    class RawRule : public RawBaseElement
    {
    public:
        RawRule(const base::parser::Location& location);

        /// get selector
        typedef base::Array<const RawSelector*> TSelectors;
        INLINE const TSelectors& selectors() const { return m_selectors; }

        //---

        /// add selector to chain
        void addSelector(const RawSelector* selector);

        /// replace the top selector
        void replaceTopSelector(const RawSelector* selector);

        // get a text representation of the rule set (CSS style)
        void print(base::IFormatStream& str) const;

    private:
        TSelectors m_selectors;
    };

    /// rule set, set of rules and properties
    class RawRuleSet : public RawBaseElement
    {
    public:
        RawRuleSet(const base::parser::Location& location);

        /// get selector chains for this rule, any must match for this rule to match
        typedef base::Array<const RawRule*> TRules;
        INLINE const TRules& rules() const { return m_rules; }

        /// get properties in the rule set
        typedef base::Array<const RawProperty*> TProperties;
        INLINE const TProperties& properties() const { return m_properties; }

        //---

        /// add rule to rule set
        void addRule(const RawRule* rule);

        /// add property
        void addProperty(const RawProperty* prop);

        // get a text representation of the rule set (CSS style)
        void print(base::IFormatStream& f) const;

    private:
        TRules m_rules;
        TProperties m_properties;
    };

    /// all rule sets collected
    class RawLibraryData
    {
    public:
        RawLibraryData(base::mem::LinearAllocator& mem);

        // get all defined rule sets
        typedef base::Array<const RawRuleSet*> TRuleSets;
        INLINE const TRuleSets& ruleSets() const { return m_ruleSets; }

        // get all defined variables
        typedef base::Array<const RawVariable*> TVariables;
        INLINE const TVariables& variables() const { return m_variables; }

        // allocator
        INLINE base::mem::LinearAllocator& allocator() const { return m_mem; }

        //--

        // add rule set
        void addRuleSet(const RawRuleSet* ruleSet);

        // add variable 
        void addVariable(const RawVariable* variable);

        // find variable by name
        const RawVariable* findVariable(base::StringID name);

        //--

        // allocate memory from internal allocator
        template< typename T, typename... Args >
        INLINE T* alloc(Args && ... args)
        {
            void* mem = m_mem.alloc(sizeof(T), __alignof(T));
            return new (mem) T(std::forward< Args >(args)...);
        }

        // copy container
        template< typename T >
        INLINE T* copyContainer(const base::Array<T>& data)
        {
            if (data.empty())
                return nullptr;

            void* mem = m_mem.alloc(data.dataSize(), __alignof(T));
            memcpy(mem, data.data(), data.dataSize());
            return (T*)mem;
        }

        // allocate string (copy) from internal allocator
        const char* allocString(base::StringView str);

        //--

        void print(base::IFormatStream& f) const;

    private:
        base::mem::LinearAllocator& m_mem;

        TRuleSets m_ruleSets;
        TVariables m_variables;

        base::HashMap<base::StringID, const RawVariable*> m_variableMap;
    };

} // prv

END_BOOMER_NAMESPACE(ui::style)