/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: variant #]
***/

#pragma once

#include "core/reflection/include/reflectionTypeName.h"
#include "reflectionMacros.h"
#include "reflectionTypeName.h"

BEGIN_BOOMER_NAMESPACE()

/// a any-type value holder similar to rtti::DataHolder but supports reflection and has a special case for small data
/// NOTE: variant itself can be serialized (thus allowing for complex map<name, variant> structures like object templates) but be sure not to have variant with variant...
TYPE_ALIGN(4, class) CORE_REFLECTION_API Variant
{
    RTTI_DECLARE_POOL(POOL_VARIANT)

public:
    static const uint32_t INTERNAL_STORAGE_SIZE = 16;

    //--

    INLINE Variant() {}
    INLINE Variant(std::nullptr_t) {}
    Variant(const Variant& other);
    Variant(Variant&& other);
    Variant(Type type, const void* initialData = nullptr); // NOTE: once the type is set it stays like that
    ~Variant();

    // copy/move
    Variant& operator=(const Variant& other);
    Variant& operator=(Variant&& other);

    // simple comparisons
    bool operator==(const Variant& other) const;
    bool operator!=(const Variant& other) const;

    //--

    // is variant empty ? (no data)
    INLINE bool empty() const { return m_type == nullptr; }

    // is variant valid?
    INLINE operator bool() const { return m_type; }

    // get assigned type
    INLINE Type type() const { return m_type; }

    // check if we are of given type
    template< typename T >
    INLINE bool is() const { return m_type == reflection::GetTypeObject<T>(); }

    //--

    // get the memory where variant data is stored
    void* data();
    const void* data() const;

    // release content
    void reset();

    // initialize EMPTY variant for type storage
    // NOTE: can only be called once
    void init(Type type, const void* contentToCopyFrom);

    //--

    // read data from a buffer and place it into variant, small type conversion may occur
    // NOTE: the variant's type is NEVER CHANGED thus if we fail to convert the data we don't store anything
    bool set(const void* srcData, Type srcType);

    // write value into a remote buffer, small type conversion may occur
    bool get(void* destData, Type destType) const;

    //--

    // try to parse value from text into this variant (will override current data)
    bool setFromString(StringView txt);

    // write text representation of the value (no debug info, pure value)
    void getAsString(IFormatStream& f) const;

    //--

    // read data from a existing value and try to place it (subject to type matching) into the current variant
    // NOTE: the variant's type is NEVER CHANGED thus if we fail to convert the data we don't store anything
    template< typename T >
    INLINE bool set(const T& srcValue)
    {
        return set(&srcValue, reflection::GetTypeObject<T>());
    }

    // write current value inside variant into the given storage, requires types to be convertible
    template< typename T >
    INLINE bool get(T& destValue) const
    {
        return get(&destValue, reflection::GetTypeObject<T>());
    }

    //--

    // get a value from variant in a safe way - a default value will be returned if something fails
    template< typename T >
    INLINE T getOrDefault(T defaultValue = T()) const
    {
        T ret = defaultValue;
        if (get(ret))
            return ret;
        else
            return defaultValue;
    }

    //--

    // get a string representation of value only (no type)
    // this is a good way to get a string for editor/tool use
    // e.g.: 
    //   5
    //   NameFromStringID
    //   AStringBuf
    //   A longer string with spaces
    //   null
    //   Mesh$engine/meshes/box.obj
    //   (x=1)(y=2)(z=3)
    void print(IFormatStream& f) const;

    // get a string representation of value + type name
    // e.g:
    //   5.0 (float)
    //   'NameFromStringID' (StringID)
    //   "AStringBuf" (StringBuf)
    //   "A longer string with spaces" (StringBuf)
    //   null (Reference to Mesh)
    //   "Mesh$engine/meshes/box.obj" (Reference to Mesh)
    //   (x=1)(y=2)(z=3) (Vector3)
    void printDebug(IFormatStream& f) const;

    //--

    // get empty variant
    static const Variant& EMPTY();

    //--

private:
    void suckFrom(Variant& other);

    union
    {
        void* ptr = nullptr;
        uint8_t bytes[INTERNAL_STORAGE_SIZE];
    } m_data;

    Type m_type = nullptr;
    bool m_allocated = false; // TODO: merge this as a bit somewhere, maybe in the "Type"

    void validate() const;
};

//--

// create a variant from a value
template< typename T>
INLINE Variant CreateVariant(const T& data)
{
    return Variant(reflection::GetTypeObject<T>(), &data);
}

//--
    
END_BOOMER_NAMESPACE()
