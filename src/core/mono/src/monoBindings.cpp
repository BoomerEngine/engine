/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "monoServiceImpl.h"

BEGIN_BOOMER_NAMESPACE()

//--

void* MonoStringFromNative(StringView txt)
{
    return mono_string_new_len(mono_domain_get(), txt.data(), txt.length());
}

void* MonoUnboxObject(void* ptr)
{
    return ptr ? mono_object_unbox((MonoObject*)ptr) : nullptr;
}

void* MonoStringFromNative(BaseStringView<wchar_t> txt)
{
    return mono_string_new_utf16(mono_domain_get(), txt.data(), txt.length());
}

void* MonoObjectFromNative(const IObject* object)
{
    return GetService<MonoServiceImpl>()->resolveScriptedObject(object);
}

//--

StringBuf MonoStringToNative(void* ptr)
{
    if (ptr)
        return StringBuf(mono_string_chars((MonoString*)ptr));
    return StringBuf();
}

StringID MonoStringIDToNative(void* ptr)
{
    if (ptr)
        return StringID(TempString("{}", mono_string_chars((MonoString*)ptr)));
    return StringID();
}

struct _MonoObjectEngineObject {
    MonoVTable* vtable;
    MonoThreadsSync* synchronisation;
    IObject* objectPtr;
};


ObjectPtr MonoObjectToNativePointer(void* ptr)
{
    if (ptr)
    {
        auto* obj = (_MonoObjectEngineObject*)ptr;
        return AddRef(obj->objectPtr);
    }

    return nullptr;
}

IObject* MonoObjectToNativePointerRaw(void* ptr)
{
    if (ptr)
    {
        auto* obj = (_MonoObjectEngineObject*)ptr;
        return obj->objectPtr;
    }

    return nullptr;
}

//---

MonoCallBase::MonoCallBase(StringID name)
    : m_name(name)
{}

void* MonoCallBase::resolveObject(IObject* engineObject)
{
    return GetService<MonoServiceImpl>()->resolveScriptedObject(engineObject);
}

bool MonoCallBase::reportException(void* exc)
{
    if (exc)
    {
        auto* excClass = mono_object_get_class((MonoObject*)exc);
        auto* name = mono_class_get_name(excClass);
        TRACE_ERROR("Unhandled MonoException: {}", name);
        return false;
    }

    return true;
}

bool MonoCallBase::callInternalNoReturn(IObject* engineObject, void** params, int paramCount)
{
    if (auto* obj = (MonoObject*)resolveObject(engineObject))
    {
        if (auto* cls = mono_object_get_class(obj))
        {
            if (auto* method = mono_class_get_method_from_name(cls, m_name.c_str(), paramCount))
            {
                MonoException* exc = nullptr;
                mono_runtime_invoke(method, obj, params, (MonoObject**) &exc);
                return reportException(exc);
            }
        }
    }

    return false;
}

void* MonoMarshalToNativeBase::unbox() const
{
    return obj ? mono_object_unbox((MonoObject*)obj) : nullptr;
}

bool MonoCallBase::callInternalReturn(IObject* engineObject, void** params, int paramCount, MonoMarshalToNativeBase& ret)
{
    if (auto* obj = (MonoObject*)resolveObject(engineObject))
    {
        if (auto* cls = mono_object_get_class(obj))
        {
            if (auto* method = mono_class_get_method_from_name(cls, m_name.c_str(), paramCount))
            {
                MonoException* exc = nullptr;
                auto* retObject = mono_runtime_invoke(method, obj, params, (MonoObject**)&exc);

                if (!reportException(exc))
                    return false;

                ret.obj = retObject;
                return true;
            }
        }
    }

    return false;
}

//---

struct TestStruct
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(TestStruct);

public:
    float x = 0.0f;
    float y = 0.0f;

    inline TestStruct(float x_=0.0f, float y_=0.0f)
        :x(x_), y(y_)
    {}

    float Length() const
    {
        return std::sqrtf(x * x + y * y);
    }
};

RTTI_BEGIN_TYPE_STRUCT(TestStruct);
    RTTI_MONO_CLASS_FUNCTION(Length);
RTTI_END_TYPE();

class TestObject : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(TestObject, IObject);

public:
    TestObject(StringView name="")
        : m_name(name)
    {}

    StringBuf m_name;

    StringBuf _GetName() const
    {
        return m_name;
    }

    void _SetName(const StringBuf& name)
    {
        TRACE_INFO("TestObject, changing name '{}' -> '{}'", m_name, name);
        m_name = name;
    }

    void SayMyName()
    {
        TRACE_INFO("May name is '{}'", m_name);
    }

    void ChangeMyName(MonoStringRef str)
    {
        TRACE_INFO("Current name: '{}'", str.txt());
        str = TempString("Senior {}", str.txt());
        TRACE_INFO("New name: '{}'", str.txt());
    }

    void GetDirection(TestStruct& dir)
    {
        TRACE_INFO("Incoming vector is {},{}", dir.x, dir.y);
        dir.x = 1;
        dir.y = 2;
    }

    void ArrayTest(MonoArrayDataT<int> arr)
    {
        TRACE_INFO("Incoming array size: {}", arr.size());

        for (uint32_t i = 0; i < arr.size(); ++i)
        {
            TRACE_INFO("Incoming array[{}]: {}", i, arr[i]);
            arr[i] = i;
        }
    }

    void ArrayObjectTest(MonoArrayObjectPtrT<TestObject> arr)
    {
        TRACE_INFO("Incoming array size: {}", arr.size());

        for (uint32_t i = 0; i < arr.size(); ++i)
        {
            if (auto obj = arr[i])
                TRACE_INFO("Incoming array[{}]: '{}'", i, obj->m_name);
        }

        arr.set(1, new TestObject("nie ma"));
    }

    void ArrayRefTest(MonoArrayRefDataT<int> arr)
    {
        arr.resize(3);
        arr[0] = 1;
        arr[1] = 2;
        arr[2] = 3;
    }

    void ArrayObjectRefTest(MonoArrayRefObjectPtrT<TestObject> arr)
    {
        arr.resize(3);
        arr.set(0, new TestObject("Ala"));
        arr.set(1, new TestObject("ma"));
        arr.set(2, new TestObject("kota"));
    }
    
    StringBuf NiceName(StringView prefix)
    {
        MonoCallBaseRet<StringBuf> caller("NiceName"_id);
        return caller.call(this, prefix);
    }

    float Tick(float dt)
    {
        MonoCallBaseRet<float> caller("Tick"_id);
        return caller.call(this, dt);
    }

    float Dot(const TestStruct& pos)
    {
        MonoCallBaseRet<float> caller("Dot"_id);
        return caller.call(this, pos);
    }

    TestStruct V2Pass(const TestStruct& pos)
    {
        MonoCallBaseRet<TestStruct> caller("VectorPassThrough"_id);
        return caller.call(this, pos);
    }

    RefPtr<TestObject> GetThis()
    {
        MonoCallBaseRet<RefPtr<TestObject>> caller("GetThis"_id);
        return caller.call(this);
    }

    RefPtr<TestObject> PPass(const IObject* obj)
    {
        MonoCallBaseRet<RefPtr<TestObject>> caller("PointerPassThrough"_id);
        return caller.call(this, obj);
    }

};

RTTI_BEGIN_TYPE_CLASS(TestObject);
    RTTI_MONO_CLASS_FUNCTION(_SetName);
    RTTI_MONO_CLASS_FUNCTION(_GetName);
    RTTI_MONO_CLASS_FUNCTION(SayMyName);
    RTTI_MONO_CLASS_FUNCTION(ChangeMyName);
    RTTI_MONO_CLASS_FUNCTION(GetDirection);
    RTTI_MONO_CLASS_FUNCTION(ArrayTest);
    RTTI_MONO_CLASS_FUNCTION(ArrayObjectTest);
    RTTI_MONO_CLASS_FUNCTION(ArrayRefTest);
    RTTI_MONO_CLASS_FUNCTION(ArrayObjectRefTest);
RTTI_END_TYPE();

namespace prv
{
    //--

    static void LogWriteLine(MonoString* str)
    {
        TRACE_INFO("Mono: {}", mono_string_chars(str));
    }

    static void LogWriteError(MonoString* str)
    {
        TRACE_ERROR("Mono: {}", mono_string_chars(str));
    }

    static void LogWriteWarning(MonoString* str)
    {
        TRACE_WARNING("Mono: {}", mono_string_chars(str));
    }

    //--

    static void ReleaseEngineObject(uint64_t engineObjectAddress)
    {
        if (auto* ptr = (IObject*)engineObjectAddress)
            ptr->releaseRef();
    }

    static MonoString* GetEngineClassName(uint64_t engineObjectAddress)
    {
        if (auto* ptr = (IObject*)engineObjectAddress)
        {
            auto className = ptr->cls()->name().view();
            return mono_string_new_len(mono_domain_get(), (char*)className.data(), className.length());
        }

        return nullptr;
    }

    static void CreateEngineObject(MonoObject* obj)
    {
        if (obj)
        {
            auto* cls = mono_object_get_class(obj);
            TRACE_INFO("Request to create engine object for Mono type '{}'", mono_class_get_name(cls));

            GetService<MonoServiceImpl>()->createEngineObjectInternalForScriptedObject(obj);
        }
    }

    //--

    void InitializeMonoBindings()
    {
        mono_add_internal_call("Boomer.Engine.Log::WriteLine", &LogWriteLine);
        mono_add_internal_call("Boomer.Engine.Log::Warning", &LogWriteWarning);
        mono_add_internal_call("Boomer.Engine.Log::Error", &LogWriteError);

        mono_add_internal_call("Boomer.Engine.IEngineObject::_ReleaseEngineObject", &ReleaseEngineObject);
        mono_add_internal_call("Boomer.Engine.IEngineObject::_GetEngineClassName", &GetEngineClassName);
        mono_add_internal_call("Boomer.Engine.IEngineObject::_CreateEngineObject", &CreateEngineObject);
    }

    //--

    bool RunSelfTest()
    {
        // run self-test
        //auto testObject = GetService<MonoService>()->createScriptedObject("Boomer.Engine.TestObject"_id);

        auto testObject = RefNew<TestObject>();
        testObject->m_name = "Ala";

        auto nice = testObject->NiceName("Dupa");
        DEBUG_CHECK_RETURN_V(nice == "Mr. Dupa", false);

        auto dot = testObject->Dot(TestStruct(1.0f, 2.0f));
        DEBUG_CHECK_RETURN_V(dot == std::sqrtf(5), false);

        auto vpass = testObject->V2Pass(TestStruct(3.0f, 5.0f));
        DEBUG_CHECK_RETURN_V(vpass.x == 3.0f, false);
        DEBUG_CHECK_RETURN_V(vpass.y == 5.0f, false);

        auto pthis = testObject->GetThis();
        DEBUG_CHECK_RETURN_V(pthis == testObject, false);

        auto ppass= testObject->PPass(testObject);
        DEBUG_CHECK_RETURN_V(ppass == testObject, false);

        float ret = testObject->Tick(0.1f);
        DEBUG_CHECK_RETURN_V(ret == 42.0f, false);
        DEBUG_CHECK_RETURN_V(testObject->m_name == "ma", false);

        testObject->Tick(0.1f);

        DEBUG_CHECK_RETURN_V(testObject->m_name == "kota", false);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        testObject->Tick(0.1f);

        return true;
    }

} // prv

//---

END_BOOMER_NAMESPACE()

