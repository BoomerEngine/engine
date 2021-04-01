/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/containers/include/stringView.h"

#include "monoHelpers.h"

BEGIN_BOOMER_NAMESPACE()

//----

/// wrap string
extern CORE_MONO_API void* MonoStringFromNative(StringView txt);

/// wrap string
extern CORE_MONO_API void* MonoObjectFromNative(const IObject* object);

/// get string from scripted string
extern CORE_MONO_API StringBuf MonoStringToNative(void* ptr);

/// get string from scripted string
extern CORE_MONO_API StringID MonoStringIDToNative(void* ptr);

/// get object pointer from a mono object
extern CORE_MONO_API ObjectPtr MonoObjectToNativePointer(void* ptr);

/// get object pointer from a mono object
extern CORE_MONO_API IObject* MonoObjectToNativePointerRaw(void* ptr);

//----

template <typename T>
struct MonoMarshalFromNative
{
    const T& val;

    inline MonoMarshalFromNative(const T& val_)
        : val(val_)
    {}

    inline void* ptr() { return (void*)&val; }
};

template <>
struct MonoMarshalFromNative<StringView>
{
    MonoMarshalFromNative(StringView val)
    {
        str = MonoStringFromNative(val);
    }

    inline void* ptr()
    {
        return str;
    }

    void* str = nullptr;
};

template <>
struct MonoMarshalFromNative<StringID>
{
    MonoMarshalFromNative(StringID val)
    {
        str = MonoStringFromNative(val.view());
    }

    inline void* ptr()
    {
        return str;
    }

    void* str = nullptr;
};

template <>
struct MonoMarshalFromNative<StringBuf>
{
    MonoMarshalFromNative(const StringBuf& val)
    {
        str = MonoStringFromNative(val.view());
    }

    inline void* ptr()
    {
        return str;
    }

    void* str = nullptr;
};

template <typename T>
struct MonoMarshalFromNative<RefPtr<T>>
{
    MonoMarshalFromNative(const RefPtr<T>& val)
    {
        obj = MonoObjectFromNative(val.view());
    }

    inline void* ptr()
    {
        return obj;
    }

    void* obj = nullptr;
};

template <typename T>
struct MonoMarshalFromNative<T*>
{
    MonoMarshalFromNative(T* val)
    {
        obj = MonoObjectFromNative(static_cast<IObject*>(val));
    }

    inline void* ptr()
    {
        return obj;
    }

    void* obj = nullptr;
};

template <typename T>
struct MonoMarshalFromNative<const T*>
{
    MonoMarshalFromNative(const T* val)
    {
        obj = MonoObjectFromNative(static_cast<const IObject*>(val));
    }

    inline void* ptr()
    {
        return obj;
    }

    void* obj = nullptr;
};


//----

struct CORE_MONO_API MonoMarshalToNativeBase
{
    void* obj = nullptr;

    void* unbox() const;

    template< typename T >
    inline T unboxT(T defaultValue = T()) const
    {
        auto* ptr = unbox();
        return ptr ? *(const T*)ptr : defaultValue;
    }
};

template <typename T>
struct MonoMarshalToNative : public MonoMarshalToNativeBase
{
    inline T val()
    {
        return unboxT<T>();
    }
};

template <>
struct MonoMarshalToNative<StringBuf> : public MonoMarshalToNativeBase
{
    inline StringBuf val() { return MonoStringToNative(obj); }
};

template <>
struct MonoMarshalToNative<StringID> : public MonoMarshalToNativeBase
{
    inline StringID val() { return MonoStringIDToNative(obj); }
};

template <typename T>
struct MonoMarshalToNative<RefPtr<T>> : public MonoMarshalToNativeBase
{
    inline RefPtr<T> val() { return rtti_cast<T>(MonoObjectToNativePointer(obj)); }
};

//----

struct CORE_MONO_API MonoCallBase
{
public:
    MonoCallBase(StringID name);

protected:
    StringID m_name;

    void* resolveObject(IObject* obj); // may create scripted object

    bool callInternalNoReturn(IObject* object, void** params, int paramCount);
    bool callInternalReturn(IObject* object, void** params, int paramCount, MonoMarshalToNativeBase& ret);

    bool reportException(void* exc);
};

//--

struct MonoCallBaseNoRet : public MonoCallBase
{
public:
    MonoCallBaseNoRet(StringID name)
        : MonoCallBase(name)
    {}

    inline void call(IObject* ptr)
    {
        callInternalNoReturn(ptr, nullptr, 0);
    }

    template< typename F1 >
    inline void call(IObject* ptr, F1 param)
    {
        MonoMarshalFromNative<typename std::remove_cv<F1>::type> f1(param);

        void* params[1];
        params[0] = f1.ptr();

        callInternalNoReturn(ptr, params, 1);
    }

};

//----

template< typename RetT >
struct MonoCallBaseRet : public MonoCallBase
{
public:
    MonoCallBaseRet(StringID name)
        : MonoCallBase(name)
    {}

    inline RetT call(IObject* ptr)
    {
        MonoMarshalToNative<RetT> ret;
        callInternalReturn(ptr, nullptr, 0, ret);
        return ret.val();
    }

    template< typename F1 >
    inline RetT call(IObject* ptr, F1 param)
    {
        MonoMarshalToNative<RetT> ret;
        MonoMarshalFromNative<typename std::remove_cv<F1>::type> f1(param);

        void* params[1];
        params[0] = f1.ptr();

        callInternalReturn(ptr, params, 1, ret);

        return ret.val();
    }

};

//--

END_BOOMER_NAMESPACE()
