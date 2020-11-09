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

        ///--

        INLINE NodePath::NodePath()
            : m_parent(nullptr)
            , m_name(nullptr)
            , m_hash(0)
        {
        }

        INLINE NodePath::~NodePath()
        {
            reset();
        }

        INLINE NodePath::NodePath(RootPathElement* parent, const NodePathPart& part, uint64_t hash)
            : m_parent(parent)
            , m_name(part)
            , m_hash(hash)
        {
            if (m_parent)
                m_parent->addRef();
        }

        INLINE NodePath::NodePath(const NodePath& other)
            : m_parent(other.m_parent)
            , m_name(other.m_name)
            , m_hash(other.m_hash)
        {
            if (m_parent)
                m_parent->addRef();
        }

        INLINE NodePath::NodePath(NodePath&& other)
            : m_parent(other.m_parent)
            , m_name(other.m_name)
            , m_hash(other.m_hash)
        {
            other.m_parent = nullptr;
            other.m_name = StringID::EMPTY();
            other.m_hash = 0;
        }

        INLINE NodePath& NodePath::operator=(const NodePath& other)
        {
            if (this != &other)
            {
                if (m_parent)
                    m_parent->releaseRef();

                m_parent = other.m_parent;
                m_name = other.m_name;
                m_hash = other.m_hash;

                if (m_parent)
                    m_parent->addRef();
            }

            return *this;
        }

        INLINE NodePath& NodePath::operator=(NodePath&& other)
        {
            if (this != &other)
            {
                if (m_parent)
                    m_parent->releaseRef();

                m_parent = other.m_parent;
                m_name = other.m_name;
                m_hash = other.m_hash;

                other.m_parent = nullptr;
                other.m_name = StringID::EMPTY();
                other.m_hash = 0;
            }

            return *this;
        }

        INLINE bool NodePath::empty() const
        {
            return m_hash == 0;
        }

        INLINE void NodePath::reset()
        {
            if (m_parent)
            {
                m_parent->releaseRef();
                m_parent = nullptr;
            }

            m_name = StringID();
            m_hash = 0;
        }

        INLINE NodePath NodePath::parent() const
        {
            if (m_parent)
                return NodePath(m_parent->m_parent, m_parent->m_name, m_parent->m_hash);
            else
                return NodePath();
        }

        INLINE const NodePathPart& NodePath::lastName() const
        {
            return m_name;
        }

        ///--

        INLINE NodePath::RootPathElement::RootPathElement(RootPathElement* parent, const NodePathPart& part, uint64_t hash)
            : m_refCount(0)
            , m_hash(hash)
            , m_parent(parent)
            , m_name(part)
        {
            if (m_parent)
                m_parent->addRef();
        }

        INLINE NodePath::RootPathElement::~RootPathElement()
        {
            if (m_parent)
                m_parent->releaseRef();
        }

        INLINE void NodePath::RootPathElement::addRef()
        {
            ++m_refCount;
        }

        INLINE void NodePath::RootPathElement::releaseRef()
        {
            if (0 == --m_refCount)
                MemDelete(this);
        }

        INLINE bool NodePath::operator==(const NodePath& other) const
        {
            if (m_hash != other.m_hash)
                return false;
            if (m_name != other.m_name)
                return false;
            return true;
        }

        INLINE bool NodePath::operator!=(const NodePath& other) const
        {
            return !operator==(other);
        }

        INLINE bool NodePath::operator<(const NodePath& other) const
        {
            return m_hash < other.m_hash;
        }

        ///--

    } // world
} // base
