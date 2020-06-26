/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: path #]
***/

#pragma once

namespace scene
{
    /// name in scene path
    typedef base::StringID NodePathPart;

    /// hierarchical path to the scene node
    /// NOTE: paths objects are just paths, they do not belong to any world
    class SCENE_COMMON_API NodePath
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

        /// get the hash of the path
        INLINE uint64_t hash() const;

        /// get the top name on the path (last name in the path)
        INLINE const NodePathPart& lastName() const;

        //---

        /// get sub path
        NodePath operator[](const NodePathPart& childName) const;

        /// get sub path
        NodePath operator[](const base::StringView<char> childName) const;

        /// get a full text representation for the whole path
        void print(base::IFormatStream& f) const;

        //---

        /// comparison
        INLINE bool operator==(const NodePath& other) const;
        INLINE bool operator!=(const NodePath& other) const;

        /// sorting
        INLINE bool operator<(const NodePath& other) const;

        //---

        /// parse path from string
        static bool Parse(const base::StringBuf& str, NodePath& outPath);

        //---

    private:
        struct RootPathElement
        {
            NodePathPart m_name;
            std::atomic<uint32_t> m_refCount;
            uint64_t m_hash;
            RootPathElement* m_parent;

            INLINE RootPathElement(RootPathElement* parent, const NodePathPart& part, uint64_t hash);
            INLINE ~RootPathElement();
            INLINE void addRef();
            INLINE void releaseRef();

            void print(base::IFormatStream& str) const;
        };

        RootPathElement* m_parent; // parent element
        NodePathPart m_name; // name of the last node
        uint64_t m_hash; // hash of the tags

        INLINE NodePath(RootPathElement* parent, const NodePathPart& part, uint64_t hash);
    };

} // scene

namespace std
{
    template <>
    struct hash<scene::NodePath>
    {
        INLINE size_t operator()(const scene::NodePath& value)
        {
            return (size_t)value.hash();
        }
    };
} //std

#include "sceneNodePath.inl"