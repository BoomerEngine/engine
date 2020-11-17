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

        /// name in scene path
        typedef StringID NodePathPart;

        /// hierarchical path to the scene node
        /// NOTE: paths objects are just paths, they do not belong to any world
        class BASE_WORLD_API NodePath
        {
        public:
            INLINE NodePath(); // empty path
            INLINE NodePath(const NodePath& other);
            INLINE NodePath(NodePath&& other);
            INLINE NodePath& operator=(const NodePath& other);
            INLINE NodePath& operator=(NodePath&& other);
            INLINE ~NodePath();

            /// is this an empty path ?
            INLINE bool empty() const;

            /// reset path to empty state
            INLINE void reset();

            /// get parent path
            INLINE NodePath parent() const;

            /// get the top name on the path (last name in the path)
            INLINE const NodePathPart& lastName() const;

            //---

            /// get sub path
            NodePath operator[](const NodePathPart& childName) const;

            /// get sub path
            NodePath operator[](const StringView childName) const;

            /// get a full text representation for the whole path
            void print(IFormatStream& f) const;

            //---

            /// comparison
            INLINE bool operator==(const NodePath& other) const;
            INLINE bool operator!=(const NodePath& other) const;

            /// sorting
            INLINE bool operator<(const NodePath& other) const;

            //---

            /// parse path from string
            static bool Parse(const StringBuf& str, NodePath& outPath);

            /// get the hash of the path
            static uint32_t CalcHash(const NodePath& path);

            //---

        private:
            struct RootPathElement : public base::mem::GlobalPoolObject<POOL_PATH_CACHE>
            {
                NodePathPart m_name;
                std::atomic<uint32_t> m_refCount;
                uint64_t m_hash;
                RootPathElement* m_parent;

                INLINE RootPathElement(RootPathElement* parent, const NodePathPart& part, uint64_t hash);
                INLINE ~RootPathElement();
                INLINE void addRef();
                INLINE void releaseRef();

                void print(IFormatStream& str) const;
            };

            RootPathElement* m_parent; // parent element
            NodePathPart m_name; // name of the last node
            uint64_t m_hash; // hash of the tags

            INLINE NodePath(RootPathElement* parent, const NodePathPart& part, uint64_t hash);
        };

    } // world
} // base

#include "worldNodePath.inl"