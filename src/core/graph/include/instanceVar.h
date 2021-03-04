/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// instance variable, placeholder for space in the instance buffer
class CORE_GRAPH_API InstanceVarBase : public NoCopy
{
public:
    InstanceVarBase(Type type);
    ~InstanceVarBase();

    /// is this bound value ?
    INLINE bool allocated() const { return m_offset != INVALID_OFFSET; }

    /// get offset in the instance buffer for this instance var
    INLINE uint32_t offset() const { return m_offset; }

    /// get type of the instance var stored
    INLINE Type type() const { return m_type; }

    /// reset variable binding
    void reset();

    /// alias this variable with other variable
    void alias(const InstanceVarBase& other);

private:
    static const uint32_t INVALID_OFFSET = 0xFFFFFF;

    uint32_t m_offset:24; // offset in the instance buffer
    Type m_type; // type of the instance var

    friend class InstanceBufferLayoutBuilder;
};

/// typed instance variable
template< typename T >
class InstanceVar : public InstanceVarBase
{
public:
    INLINE InstanceVar()
        : InstanceVarBase(GetTypeObject<T>())
    {}
};

/// typed instance array
class InstanceArrayBase : public NoCopy
{
public:
    InstanceArrayBase(Type type);

    /// get offset in the instance buffer for this instance var
    INLINE uint32_t offset() const { return m_offset; }

    /// get type of the instance var stored
    INLINE Type type() const { return m_type; }

    /// get allocated array size
    INLINE uint32_t size() const { return m_size; }

private:
    uint32_t m_offset; // offset in the instance buffer
    uint32_t m_size; // size of the array - number of elements
    Type m_type; // type of the instance var

    friend class InstanceBufferLayoutBuilder;
};

/// typed instance variable
template< typename T >
class InstanceArray : public InstanceArrayBase
{
public:
    INLINE InstanceArray()
        : InstanceArrayBase(GetTypeObject<T>())
    {}
};

END_BOOMER_NAMESPACE()
