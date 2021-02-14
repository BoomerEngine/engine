/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

namespace base
{
    namespace world
    {

        //--

        /// builder of hierarchical path to the scene node
        class BASE_WORLD_API NodePathBuilder
        {
        public:
            NodePathBuilder(StringView layerDepotPath = ""); // we must start with layer path
            NodePathBuilder(const NodePathBuilder& other) = default;
            NodePathBuilder& operator=(const NodePathBuilder& other) = default;
            ~NodePathBuilder();

            /// is this an empty path ?
            INLINE bool empty() const;

            /// reset path to empty state
            INLINE void reset();

            /// get the top name on the path (last name in the path)
            INLINE StringID lastName() const;

            //---

            /// pop last ID
            void pop();

            /// push name, pushing ".." pops
            void pushSingle(StringID id);

            /// push path, separated by "/", if path starts with "/" it's considered absolute, "$" also is an absolute ID
            void pushPath(StringView path);

            /// get parent path
            NodePathBuilder parent() const;

            /// get a full text representation for the whole path
            void print(IFormatStream& f) const;

            //---

            /// get a string path
            StringBuf toString() const;

            /// calculate node ID from current path
            NodeGlobalID toID() const;

            //---

        private:
            InplaceArray<StringID, 16> m_parts;
        };

        //--

        INLINE bool NodePathBuilder::empty() const
        {
            return m_parts.empty();
        }

        INLINE void NodePathBuilder::reset()
        {
            m_parts.reset();
        }

        INLINE StringID NodePathBuilder::lastName() const
        {
            return !m_parts.empty() ? m_parts.back() : StringID::EMPTY();
        }

    } // world
} // base
