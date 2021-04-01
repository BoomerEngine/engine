/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

struct MonoStringRef : public NoCopy
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MonoStringRef);

public:
    MonoStringRef(void** str_);
    MonoStringRef(MonoStringRef&& other);
    MonoStringRef& operator==(MonoStringRef&& other);

    StringBuf txt() const;
    StringID id() const;

    BaseStringView<wchar_t> view() const;

    void txt(StringView txt);

    void operator=(StringView txt);

private:
    void** strRef = nullptr;
};

//---

struct MonoArrayData : public NoCopy
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MonoArrayData);

public:
    MonoArrayData(void* ar_);
    MonoArrayData(MonoArrayData&& other);
    MonoArrayData& operator==(MonoArrayData&& other);

    uint32_t size();
    //void resize(uint32_t size);

    //void clear();

    void* ptr(uint32_t index, uint32_t size);
    const void* ptr(uint32_t index, uint32_t size) const;

protected:
    void* ar = nullptr;

    IObject* objectPtr(uint32_t index) const;
    void setObjectPtr(uint32_t index, IObject* ptr);
};

//---

struct MonoArrayRefData : public NoCopy
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MonoArrayRefData);

public:
    MonoArrayRefData(void** ar_, Type t);
    MonoArrayRefData(MonoArrayRefData&& other);
    MonoArrayRefData& operator==(MonoArrayRefData&& other);

    uint32_t size();
    void resize(uint32_t size);
    void nullify(); 

    void* ptr(uint32_t index, uint32_t size);
    const void* ptr(uint32_t index, uint32_t size) const;

protected:
    void** arRef = nullptr;
    void* arKlass = nullptr;

    IObject* objectPtr(uint32_t index) const;
    void setObjectPtr(uint32_t index, IObject* ptr);
};

//---

template< typename T >
struct MonoArrayDataT : public MonoArrayData
{
public:
    inline MonoArrayDataT(void* ar_)
        : MonoArrayData(ar_)
    {}

    inline MonoArrayDataT(MonoArrayDataT<T>&& other)
        : MonoArrayData(other)
    {}

    inline MonoArrayDataT& operator==(MonoArrayDataT&& other)
    {
        MonoArrayData::operator==(std::move(other));
        return *this;
    }

    inline T& operator[](uint32_t index)
    {
        return *(T*)ptr(index, sizeof(T));
    }

    inline const T& operator[](uint32_t index) const
    {
        return *(const T*)ptr(index, sizeof(T));
    }
};

//---

template< typename T >
struct MonoArrayObjectPtrT : public MonoArrayData
{
public:
    inline MonoArrayObjectPtrT(void* ar_)
        : MonoArrayData(ar_)
    {}

    inline MonoArrayObjectPtrT(MonoArrayDataT<T>&& other)
        : MonoArrayData(other)
    {}

    inline MonoArrayObjectPtrT& operator==(MonoArrayObjectPtrT&& other)
    {
        MonoArrayData::operator==(std::move(other));
        return *this;
    }

    inline T* operator[](uint32_t index) const
    {
        return rtti_cast<T>(objectPtr(index));
    }

    inline void set(uint32_t index, T* ptr)
    {
        return setObjectPtr(index, ptr);
    }
};

//---

template< typename T >
struct MonoArrayRefDataT : public MonoArrayRefData
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MonoArrayRefData);

public:
    inline MonoArrayRefDataT(void** ar_)
        : MonoArrayRefData(ar_, GetTypeObject<T>())
    {}

    inline MonoArrayRefDataT(MonoArrayRefDataT<T>&& other)
        : MonoArrayRefData(other)
    {}

    inline MonoArrayRefDataT& operator==(MonoArrayRefDataT&& other)
    {
        MonoArrayRefData::operator==(std::move(other));
        return *this;
    }

    inline T& operator[](uint32_t index)
    {
        return *(T*)ptr(index, sizeof(T));
    }

    inline const T& operator[](uint32_t index) const
    {
        return *(const T*)ptr(index, sizeof(T));
    }
};

//---

template< typename T >
struct MonoArrayRefObjectPtrT : public MonoArrayRefData
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MonoArrayRefData);

public:
    inline MonoArrayRefObjectPtrT(void** ar_)
        : MonoArrayRefData(ar_, GetTypeObject<T>())
    {}

    inline MonoArrayRefObjectPtrT(MonoArrayRefObjectPtrT<T>&& other)
        : MonoArrayRefData(other)
    {}

    inline MonoArrayRefObjectPtrT& operator==(MonoArrayRefObjectPtrT&& other)
    {
        MonoArrayRefData::operator==(std::move(other));
        return *this;
    }

    inline T* operator[](uint32_t index) const
    {
        return rtti_cast<T>(objectPtr(index));
    }

    inline void set(uint32_t index, T* ptr)
    {
        return setObjectPtr(index, ptr);
    }
};

//--

namespace resolve
{
    template<typename T>
    struct TypeName<MonoArrayDataT<T>>
    {
        static StringID GetTypeName()
        {
            return boomer::GetTypeName<MonoArrayData>();
        }
    };

    template<typename T>
    struct TypeName<MonoArrayObjectPtrT<T>>
    {
        static StringID GetTypeName()
        {
            return boomer::GetTypeName<MonoArrayData>();
        }
    };

    template<typename T>
    struct TypeName<MonoArrayRefDataT<T>>
    {
        static StringID GetTypeName()
        {
            return boomer::GetTypeName<MonoArrayRefData>();
        }
    };

    template<typename T>
    struct TypeName<MonoArrayRefObjectPtrT<T>>
    {
        static StringID GetTypeName()
        {
            return boomer::GetTypeName<MonoArrayRefData>();
        }
    };

} // resolve

//--

END_BOOMER_NAMESPACE()
