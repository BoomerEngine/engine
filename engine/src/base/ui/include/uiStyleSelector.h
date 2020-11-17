/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once

namespace ui
{
    namespace style
    {
        /// ID of the node in the selector matching tree
        typedef uint16_t SelectorID;

        /// ID of the value table (param tables are compacted)
        typedef uint16_t ValueID;

        /// selector evaluation context, used to match selectors that apply to the given attributes
        class BASE_UI_API SelectorMatchContext : public base::mem::GlobalPoolObject<POOL_UI>
        {
        public:
            SelectorMatchContext(base::StringID id, base::SpecificClassType<IElement> elementClass = nullptr);

            INLINE uint64_t key() const { return m_key; }

            INLINE base::StringID id() const { return m_id; }

            INLINE const base::StringID* classes() const { return m_classes.typedData(); }
            INLINE const uint32_t numClasses() const { return m_classes.size(); }

            INLINE const base::StringID* pseudoClasses() const { return m_pseudoClasses.typedData(); }
            INLINE const uint32_t numPseudoClasses() const { return m_pseudoClasses.size(); }

            INLINE const base::StringID* types() const { return m_types.typedData(); }
            INLINE const uint32_t numTypes() const { return m_types.size(); }

            //--

            bool id(base::StringID name);

            bool hasClass(base::StringID name) const;
            bool addClass(base::StringID name);
            bool removeClass(base::StringID name);

            bool hasPseudoClass(base::StringID name) const;
            bool addPseudoClass(base::StringID name);
            bool removePseudoClass(base::StringID name);

            bool customType(base::StringID type);
            base::StringID customType() const;

            //--

            // debug print
            void print(base::IFormatStream& f) const;

        private:
            static const auto MAX_TYPES = 8;
            static const auto MAX_CLASSES = 6;
            static const auto MAX_PSEUDO_CLASSES = 4;

            uint64_t m_key = 0;

            bool m_hasCustomTypeName = false; // TODO: remove

            // TODO: refactor storage to use one array and ranges in it

            base::StringID m_id;

            typedef base::InplaceArray<base::StringID, MAX_TYPES> TTypes;
            TTypes m_types;

            typedef base::InplaceArray<base::StringID, MAX_CLASSES> TClasses;
            TClasses m_classes;

            typedef base::InplaceArray<base::StringID, MAX_PSEUDO_CLASSES> TPseudoClasses;
            TPseudoClasses m_pseudoClasses;

            bool updateKey();
        };

        /// set of matched selectors that satisfied the SelectorMatchContext
        /// the matched selectors are basis of determining which rule set to apply
        class BASE_UI_API SelectorMatch : public base::mem::GlobalPoolObject<POOL_UI_STYLES>
        {
        public:
            SelectorMatch();
            SelectorMatch(base::Array<SelectorID>&& ids, uint64_t hash);
            SelectorMatch(const SelectorMatch& other) = default;
            SelectorMatch(SelectorMatch&& other) = default;
            SelectorMatch& operator=(const SelectorMatch& other) = default;
            SelectorMatch& operator=(SelectorMatch&& other) = default;

            // is the match empty ? If so there's no styling available :(
            INLINE bool empty() const { return m_matchedSelectors.empty(); }

            // get entries (ordered set)
            typedef base::Array<SelectorID> TMatchedSelectors;
            INLINE const TMatchedSelectors& selectors() const { return m_matchedSelectors; }

            // get hash
            INLINE uint64_t hash() const { return m_hash; }

            //--

            // get the universal empty selector match
            const static SelectorMatch& EMPTY();

        private:
            TMatchedSelectors m_matchedSelectors; // sorted array with IDs of the matched rules
            uint64_t m_hash; // hash of the values used to select the styling here
        };

        /// selector combinator type
        /// determines the relation between rules
        enum class SelectorCombinatorType : uint8_t
        {
            AnyParent, // Descendant Selector, the hierarchical check will ensure that the element is one of the parents of tested element
            DirectParent, // Child Selector, the hierarchical check will pass only if the element is DIRECT PARENT of the tested element
            AdjecentSibling, // the hierarchical check will pass only if the tested element is following element in the same parent
            GeneralSibling, // the hierarchical check will pass only if the element is another child of the same parent
        };

        /// selector value
        class BASE_UI_API SelectorParam
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SelectorParam);

        public:
            INLINE SelectorParam() {};
            INLINE SelectorParam(base::StringID param, ValueID value)
                : m_paramName(param)
                , m_valueId(value)
            {}

            /// get described style parameter
            INLINE base::StringID paramName() const { return m_paramName; }

            /// get assigned value of parameter
            INLINE ValueID valueId() const { return m_valueId; }

        private:
            base::StringID m_paramName; // ID of the parameter in the parameter table
            ValueID m_valueId = 0; // ID of the value in the value table
        };

        /// selector match node
        class BASE_UI_API SelectorMatchParams
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SelectorMatchParams);

        public:
            SelectorMatchParams();
            SelectorMatchParams(const base::StringID type, const base::StringID clas, const base::StringID clas2, const base::StringID id, const base::StringID pseudoClass, const base::StringID pseudoClass2);

            //---
            
            /// test against a match context
            bool matches(const SelectorMatchContext& ctx) const;

            /// debug print
            void print(base::IFormatStream& f) const;

            /// calculate hash
            void calcHash(base::CRC64& crc) const;

        private:
            base::StringID m_type;
            base::StringID m_id;
            base::StringID m_pseudoClass;
            base::StringID m_pseudoClass2;
            base::StringID m_class;
            base::StringID m_class2;
        };

        /// node in the selector tree
        /// selector hierarchy is transformed into a tree
        class BASE_UI_API SelectorNode
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SelectorNode);

        public:
            SelectorNode();
            SelectorNode(SelectorID parentNodeId, const base::Array<SelectorParam>& params, const SelectorMatchParams& matchParameters, SelectorCombinatorType combinator);

            /// get the relation toward the parent node
            INLINE SelectorCombinatorType relation() const { return m_combinator; }

            /// get ID of the parent node
            INLINE SelectorID parentId() const { return m_parentId; }

            /// get the match parameters for this selector
            INLINE const SelectorMatchParams& matchParameters() const { return m_matchParameters; }

            /// get the parameters defined at this selector
            typedef base::Array<SelectorParam> TParameters;
            INLINE const TParameters& parameters() const { return m_parameters; }

        private:
            SelectorCombinatorType m_combinator;
            SelectorID m_parentId;
            SelectorMatchParams m_matchParameters;
            TParameters m_parameters;
        };


    } // style
} // ui
