/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// builder of hierarchical path to the scene node
class ENGINE_WORLD_API EntityStaticIDBuilder
{
public:
    EntityStaticIDBuilder(StringView layerDepotPath = ""); // we must start with layer path
    EntityStaticIDBuilder(const EntityStaticIDBuilder& other) = default;
    EntityStaticIDBuilder& operator=(const EntityStaticIDBuilder& other) = default;
    ~EntityStaticIDBuilder();

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
    EntityStaticIDBuilder parent() const;

    /// get a full text representation for the whole path
    void print(IFormatStream& f) const;

    //---

    /// get a string path
    StringBuf toString() const;

    /// calculate node ID from current path
    EntityStaticID toID() const;

    //---

    /// build from full path
    static EntityStaticID CompileFromPath(StringView path);

    //---

private:
    InplaceArray<StringID, 16> m_parts;
};

//--

INLINE bool EntityStaticIDBuilder::empty() const
{
    return m_parts.empty();
}

INLINE void EntityStaticIDBuilder::reset()
{
    m_parts.reset();
}

INLINE StringID EntityStaticIDBuilder::lastName() const
{
    return !m_parts.empty() ? m_parts.back() : StringID::EMPTY();
}

END_BOOMER_NAMESPACE()
