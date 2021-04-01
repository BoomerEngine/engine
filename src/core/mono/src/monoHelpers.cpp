/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "monoServiceImpl.h"
#include "monoHelpers.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MonoStringRef);
RTTI_END_TYPE();

MonoStringRef::MonoStringRef(void** str_)
    : strRef(str_)
{}

MonoStringRef::MonoStringRef(MonoStringRef&& other)
{
    strRef = other.strRef;
    other.strRef = nullptr;
}

MonoStringRef& MonoStringRef::operator==(MonoStringRef&& other)
{
    strRef = other.strRef;
    other.strRef = nullptr;
    return *this;
}

StringBuf MonoStringRef::txt() const
{
    if (strRef)
        if (auto* str = *(MonoString**)strRef)
            return StringBuf(mono_string_chars(str));

    return "";
}

StringID MonoStringRef::id() const
{
    if (strRef)
        if (auto* str = *(MonoString**)strRef)
            return StringID(TempString("{}", mono_string_chars(str)));

    return StringID();
}

BaseStringView<wchar_t> MonoStringRef::view() const
{
    if (strRef)
        if (auto* str = *(MonoString**)strRef)
            return mono_string_chars(str);

    return L"";
}

void MonoStringRef::txt(StringView txt)
{
    if (strRef)
    {
        MonoString*& str = *(MonoString**)strRef;

        if (txt)
            str = mono_string_new_len(mono_domain_get(), txt.data(), txt.length());
        else
            str = nullptr;
    }
}

void MonoStringRef::operator=(StringView txt_)
{
    txt(txt_);
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MonoArrayData);
RTTI_END_TYPE();

MonoArrayData::MonoArrayData(void* ar_)
    : ar(ar_)
{}

MonoArrayData::MonoArrayData(MonoArrayData&& other)
{
    ar = other.ar;
    other.ar = nullptr;
}

MonoArrayData& MonoArrayData::operator==(MonoArrayData&& other)
{
    if (this != &other)
    {
        ar = other.ar;
        other.ar = nullptr;
    }

    return *this;
}

uint32_t MonoArrayData::size()
{
    return ar ? mono_array_length((MonoArray*)ar) : 0;
}

void* MonoArrayData::ptr(uint32_t index, uint32_t size)
{
    return ar ? mono_array_addr_with_size((MonoArray*)ar, size, index) : nullptr;
}

const void* MonoArrayData::ptr(uint32_t index, uint32_t size) const
{
    return ar ? mono_array_addr_with_size((MonoArray*)ar, size, index) : nullptr;
}

IObject* MonoArrayData::objectPtr(uint32_t index) const
{
    if (ar)
    {
        auto obj = *(MonoObject**)mono_array_addr_with_size((MonoArray*)ar, sizeof(MonoObject*), index);
        return MonoObjectToNativePointerRaw(obj);
    }

    return nullptr;
}

void MonoArrayData::setObjectPtr(uint32_t index, IObject* ptr)
{
    if (ar)
    {
        auto** obj = (void**)mono_array_addr_with_size((MonoArray*)ar, sizeof(MonoObject*), index);
        *obj = MonoObjectFromNative(ptr);
    }
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MonoArrayRefData);
RTTI_END_TYPE();

MonoArrayRefData::MonoArrayRefData(void** ar_, Type t)
    : arRef(ar_)
{
    arKlass = GetService<MonoServiceImpl>()->findMonoClassForType(t);
    ASSERT_EX(arKlass != nullptr, "Unknown engine type for array");
}

MonoArrayRefData::MonoArrayRefData(MonoArrayRefData&& other)
{
    arRef = other.arRef;
    other.arRef = nullptr;
}

MonoArrayRefData& MonoArrayRefData::operator==(MonoArrayRefData&& other)
{
    if (this != &other)
    {
        arRef = other.arRef;
        other.arRef = nullptr;
    }

    return *this;
}

uint32_t MonoArrayRefData::size()
{
    if (arRef)
        if (auto* ar = *(MonoArray**)arRef)
            return mono_array_length(ar);

    return 0;
}

void MonoArrayRefData::resize(uint32_t size)
{
    if (arRef)
    {
        auto*& ar = *(MonoArray**)arRef;
        if (ar && mono_array_length(ar) == size)
            return;

        ar = mono_array_new(mono_domain_get(), (MonoClass*)arKlass, size);
    }
}

void MonoArrayRefData::nullify()
{
    if (arRef)
    {
        auto*& ar = *(MonoArray**)arRef;
        ar = nullptr;
    }
}

void* MonoArrayRefData::ptr(uint32_t index, uint32_t size)
{
    if (arRef)
        if (auto* ar = *(MonoArray**)arRef)
            return mono_array_addr_with_size((MonoArray*)ar, size, index);

    return nullptr;
}

const void* MonoArrayRefData::ptr(uint32_t index, uint32_t size) const
{
    if (arRef)
        if (auto* ar = *(MonoArray**)arRef)
            return mono_array_addr_with_size((MonoArray*)ar, size, index);

    return nullptr;
}

IObject* MonoArrayRefData::objectPtr(uint32_t index) const
{
    if (arRef)
    {
        if (auto* ar = *(MonoArray**)arRef)
        {
            auto obj = *(MonoObject**)mono_array_addr_with_size((MonoArray*)ar, sizeof(MonoObject*), index);
            return MonoObjectToNativePointerRaw(obj);
        }
    }

    return nullptr;
}

void MonoArrayRefData::setObjectPtr(uint32_t index, IObject* ptr)
{
    if (arRef)
    {
        if (auto* ar = *(MonoArray**)arRef)
        {
            auto** obj = (void**)mono_array_addr_with_size((MonoArray*)ar, sizeof(MonoObject*), index);
            *obj = MonoObjectFromNative(ptr);
        }
    }
}

END_BOOMER_NAMESPACE()

