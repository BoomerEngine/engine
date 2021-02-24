/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\function #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::rtti)

//---

class IFunctionStackFrame;

// "calling convention" for scripted functions
struct FunctionCallingParams
{
    static const uint32_t MAX_ARGS = 16;

    // where should we write the return value, points to memory location prepared for return type T
    // NOTE: this may be NULL if we are not interested in the return value
    void* m_returnPtr;

    // memory location of the argument DATA
    // if function argument is a reference this will be the actual place to suck the data
    // if function argument is passed by value we will have to get value from the memory at this location
    void* m_argumentsPtr[MAX_ARGS];

    //--

    template< typename T >
    INLINE void writeResult(T val) const
    {
        *(typename std::remove_cv<T*>::type)m_returnPtr = std::move(val);
    }

    template< typename T, uint32_t Arg >
    INLINE T arg() const
    {
        typedef typename std::remove_reference< typename std::remove_pointer<T>::type >::type InnerT;
        typedef typename std::remove_cv<InnerT>::type SafeT;

        return *(InnerT*) m_argumentsPtr[Arg];
    }
};

//---

#ifdef PLATFORM_MSVC
class UnknownClass {};
typedef void(__cdecl UnknownClass::* TMethodPtr)();
#else
class UnknownClass;
typedef void (UnknownClass::* TMethodPtr)();
#endif
typedef void (*TFunctionPtr)();

// pointer to a function, supports different types
// based on AngleScript implementation
struct FunctionPointer
{
    INLINE FunctionPointer(uint8_t f = 0)
    {
        memzero(ptr.dummy, sizeof(ptr.dummy));
        flag = f;
    }

    INLINE operator bool() const
    {
        return flag != 0;
    }

    void CopyMethodPtr(const void *mthdPtr, size_t size)
    {
        for( size_t n = 0; n < size; n++ )
            ptr.dummy[n] = reinterpret_cast<const char *>(mthdPtr)[n];
    }

    union
    {
        // The largest known method point is 20 bytes (MSVC 64bit),
        // but with 8byte alignment this becomes 24 bytes. So we need
        // to be able to store at least that much.
        char dummy[25];
        struct {TMethodPtr mthd; char dummy[25-sizeof(TMethodPtr)];} m;
        struct {TFunctionPtr func; char dummy[25-sizeof(TFunctionPtr )];} f;
    } ptr;

    uint8_t flag; // 1 = generic, 2 = global func, 3 = method
};

//---

//---

template <class T>
INLINE FunctionPointer MakeFunctionHelper(T func)
{
    FunctionPointer p(2);

#ifdef PLATFORM_64BIT
    p.ptr.f.func = reinterpret_cast<TFunctionPtr>(size_t(func));
#else
    p.ptr.f.func = reinterpret_cast<TFunctionPtr>(func);
#endif
    return p;
}

typedef void (UnknownClass::*TSimpleMethod)();
static const int SINGLE_PTR_SIZE = sizeof(TSimpleMethod);

template <int N>
struct MakeMethodHelper
{
    template<class M>
    static FunctionPointer Convert(M Mthd)
    {
        int ERROR_UnsupportedMethodPtr[N-100];

        FunctionPointer p(0);
        return p;
    }
};

template <>
struct MakeMethodHelper<SINGLE_PTR_SIZE>
{
    template<class M>
    static FunctionPointer Convert(M Mthd)
    {
        FunctionPointer p(3);
        p.CopyMethodPtr(&Mthd, SINGLE_PTR_SIZE);
        return p;
    }
};

template <>
struct MakeMethodHelper<SINGLE_PTR_SIZE+1*sizeof(int)>
{
    template <class M>
    static FunctionPointer Convert(M Mthd)
    {
        FunctionPointer p(3);
        p.CopyMethodPtr(&Mthd, SINGLE_PTR_SIZE+sizeof(int));
        return p;
    }
};

template <>
struct MakeMethodHelper<SINGLE_PTR_SIZE+2*sizeof(int)>
{
    template <class M>
    static FunctionPointer Convert(M Mthd)
    {
        FunctionPointer p(3);
        p.CopyMethodPtr(&Mthd, SINGLE_PTR_SIZE+2*sizeof(int));

        // Microsoft has a terrible optimization on class methods with virtual inheritance.
        // They are hardcoding an important offset, which is not coming in the method pointer.

#if defined(PLATFORM_MSVC) && !defined(PLATFORM_64BIT)
        // Method pointers for virtual inheritance is not supported,
            // as it requires the location of the vbase table, which is
            // only available to the C++ compiler, but not in the method
            // pointer.

            // You can get around this by forward declaring the class and
            // storing the sizeof its method pointer in a constant. Example:

            // class ClassWithVirtualInheritance;
            // int ClassWithVirtualInheritance_workaround = sizeof(void ClassWithVirtualInheritance::*());

            // This will force the compiler to use the unknown type
            // for the class, which falls under the next case


            // Copy the virtual table index to the 4th dword so that AngelScript
            // can properly detect and deny the use of methods with virtual inheritance.
            *(reinterpret_cast<uint32_t*>(&p)+3) = *(reinterpret_cast<uint32_t*>(&p)+2);
#endif

        return p;
    }
};

template <>
struct MakeMethodHelper<SINGLE_PTR_SIZE+3*sizeof(int)>
{
    template <class M>
    static FunctionPointer Convert(M Mthd)
    {
        FunctionPointer p(3);
        p.CopyMethodPtr(&Mthd, SINGLE_PTR_SIZE+3*sizeof(int));
        return p;
    }
};

template <>
struct MakeMethodHelper<SINGLE_PTR_SIZE+4*sizeof(int)>
{
    template <class M>
    static FunctionPointer Convert(M Mthd)
    {
        // On 64bit platforms with 8byte data alignment
        // the unknown class method pointers will come here.

        FunctionPointer p(3);
        p.CopyMethodPtr(&Mthd, SINGLE_PTR_SIZE+4*sizeof(int));
        return p;
    }
};

//---

// native calling wrapper pointer
typedef void (*TFunctionWrapperPtr)(void* contextPtr, const FunctionPointer& func, const FunctionCallingParams& params);

//---

// JITed function wrapper pointer
typedef void (*TFunctionJittedWrapperPtr)(void* context, const IFunctionStackFrame* stackFrame, const rtti::FunctionCallingParams* params);

//---

END_BOOMER_NAMESPACE(base::rtti)

#define MakeMethodPtr(c,m) base::rtti::MakeMethodHelper<sizeof(void (c::*)())>::Convert((void (c::*)())(&c::m))
#define MakeFunctionPtr(f) base::rtti::MakeFunctionHelper(f)