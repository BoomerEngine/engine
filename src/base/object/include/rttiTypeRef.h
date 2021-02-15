/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#pragma once

#include "rttiType.h"
#include "rttiClassRef.h"

namespace base
{
 
    class ClassType;

    template< typename T >
    class SpecificClassType;

    /// a reference to rtti type, basically just a pointer wrapper with extra steps :)
    /// NOTE: this is not serializable via RTTI system (has no type) but it can be serialized manually to stream
    class BASE_OBJECT_API Type
    {
    public:
        INLINE Type() : m_type(nullptr) {}
        INLINE Type(const rtti::IType* typeRef) : m_type(typeRef) {}
        INLINE Type(std::nullptr_t) : m_type(nullptr) {}

        INLINE Type(const Type& other) = default;
        INLINE Type(Type&& other) = default;
        INLINE Type& operator=(const Type& other) = default;
        INLINE Type& operator=(Type&& other) = default;
        INLINE ~Type() = default;

        // assign from type
        INLINE Type& operator=(const rtti::IType* other)
        {
            m_type = other;
            return *this;
        }

        // assign from class types
        INLINE Type& operator=(const ClassType& other);
        INLINE Type(const ClassType& other);

        // assign from specific class types
        template< typename T >
        INLINE Type& operator=(const SpecificClassType<T>& other);
        template< typename T >
        INLINE Type(const SpecificClassType<T>& other);

        // simple comparisons
        INLINE bool operator==(const Type& other) const { return m_type == other.m_type; }
        INLINE bool operator!=(const Type& other) const { return m_type != other.m_type; }
        INLINE bool operator==(const rtti::IType* other) const { return m_type == other; }
        INLINE bool operator!=(const rtti::IType* other) const { return m_type != other; }


        //! is this an empty type reference ?
        INLINE bool empty() const { return m_type == nullptr; }

        //! cast to bool - true if not null
        INLINE operator bool() const { return m_type != nullptr; }

        // get referenced type
        INLINE const rtti::IType* ptr() const { return m_type; }

        // get referenced type directly
        INLINE const rtti::IType* operator->() const { return m_type; }

        // get the "meta type" of this type - describes what kind of type this type is
        // NOTE: for empty types this returns "void"
        INLINE rtti::MetaType metaType() const { return m_type ? m_type->metaType() : rtti::MetaType::Void; }

        // get name of the type, returns empty name of empty type ref
        INLINE StringID name() const { return m_type ? m_type->name() : StringID(); }

        //----

        // is this a class type ? (toClass will work)
        INLINE bool isClass() const { return metaType() == rtti::MetaType::Class; }

        // is this an array type ?
        INLINE bool isArray() const { return metaType() == rtti::MetaType::Array; }

        // is this an enum type ?
        INLINE bool isEnum() const { return metaType() == rtti::MetaType::Enum; }

        // is this an bitfield type ?
        INLINE bool isBitfield() const { return metaType() == rtti::MetaType::Bitfield; }

        // is this a simple data type?
        INLINE bool isSimple() const { return metaType() == rtti::MetaType::Simple; }

        // is this a handle type ?
        INLINE bool isHandle() const { return metaType() == rtti::MetaType::StrongHandle || metaType() == rtti::MetaType::WeakHandle; }

        // is this a strong (shared) handle ?
        INLINE bool isStrongHandle() const { return metaType() == rtti::MetaType::StrongHandle; }

        // is this a weak handle ?
        INLINE bool isWeakHandle() const { return metaType() == rtti::MetaType::WeakHandle; }

        // is this a resource reference type ?
        INLINE bool isResourceRef() const { return metaType() == rtti::MetaType::ResourceRef; }

        //----

        // convert to a class type
        // NOTE: returns NULL if type is not a class
        ClassType toClass() const;

        // convert to a class type of at least specific class
        template< typename T >
        INLINE SpecificClassType<T> toSpecificClass() const;

        // get internal type - works for any type that has internal type (array, handle, class ref, etc)
        // NOTE: returns empty Type() if type does not have internal type
        Type innerType() const;

        // get referenced class - works for types that are typed themselves by a class: ClassRef<>, Handle<>, etc
        // NOTE: returns empty ClassType() if type does not have internal class
        ClassType referencedClass() const;

        //--

        // print a type debug info
        void print(base::IFormatStream& f) const;

        // hash calculation
        static uint32_t CalcHash(const Type& type);

        // hash calculation
        static uint32_t CalcHash(const StringID& type);

        //--

        // get empty type reference
        static const Type& EMPTY();

        //--
        
    private:
        const rtti::IType* m_type = nullptr;
    };

    //--

    template< typename T >
    INLINE SpecificClassType<T> Type::toSpecificClass() const
    {
        return toClass().cast<T>();
    }

    INLINE Type& Type::operator=(const ClassType& other)
    {
        m_type = (const rtti::IType*)other.ptr();
        return *this;
    }

    INLINE Type::Type(const ClassType& other)
        : m_type((const rtti::IType*)other.ptr())
    {}

    template< typename T >
    INLINE Type& Type::operator=(const SpecificClassType<T>& other)
    {
        m_type = (const rtti::IType*)other.ptr();
        return *this;
    }

    template< typename T >
    INLINE Type::Type(const SpecificClassType<T>& other)
        : m_type((const rtti::IType*)other.ptr())
    {}

    //--

} // base
