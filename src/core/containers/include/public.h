/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

//----

// Glue headers and logic
#include "core_containers_glue.inl"

// All shared pointer stuff is always exposed
#include "uniquePtr.h"
#include "refCounted.h"
#include "refPtr.h"
#include "refWeakPtr.h"

// Common shit
#include "flags.h"
#include "crc.h"
#include "stringView.h"
#include "stringVector.h"
#include "stringID.h"
#include "stringBuf.h"
#include "stringBuilder.h"
#include "stringParser.h"

// Inlined stuff
#include "stringID.inl"
#include "stringBuf.inl"
#include "stringView.inl"
#include "stringVector.inl"
#include "crc.inl"

//----

BEGIN_BOOMER_NAMESPACE()

class RectAllocator;

struct CompileTimeCRC32;
struct CompileTimeCRC64;
class IClipboardHandler;

//----

template< typename T, typename... Args >
INLINE RefPtr<T> RefNew(Args&& ... args)
{
    static_assert(std::is_base_of<IReferencable, T>::value, "RefNew can only be used with IReferencables");
    return NoAddRef(new T(std::forward< Args >(args)...));
}

//----

// decode Base64 data
extern CORE_CONTAINERS_API void* DecodeBase64(const char* startTxt, const char* endTxt, uint32_t& outDataSize, const PoolTag& poolID = POOL_MEM_BUFFER);

// decode an escaped C string, supported escapements: \n \r \t \b \0 \x \" \' \\
// NOTE: optionally we can automatically eat the quotes around the string
// NOTE: returned buffer is NOT zero terminated, use a StringView to access
extern CORE_CONTAINERS_API char* DecodeCString(const char* startTxt, const char* endTxt, uint32_t& outDataSize, const PoolTag& poolID = POOL_MEM_BUFFER);

//----

// basic progress reporter
class CORE_CONTAINERS_API IProgressTracker : public NoCopy
{
public:
    virtual ~IProgressTracker();

    /// check if we were canceled, if so we should exit any inner loops we are in
    virtual bool checkCancelation() const = 0;

    /// post status update, will replace the previous status update
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) = 0;

    /// post status update without any numerical information (just a sliding bar)
    INLINE void reportProgress(StringView text) { reportProgress(0, 0, text); }

    //----

    // get an empty progress tracker - will not print anything
    static IProgressTracker& DevNull();
};

//--

template< typename T >
INLINE static NoAddRefWrapper<T> NoAddRef(T* ptr)
{
    //static_assert(std::is_base_of<IReferencable, T>::value, "Type should be based on IReferencable");
    return NoAddRefWrapper(ptr);
}

template< typename T >
INLINE static NoAddRefWrapper<T> NoAddRef(const T* ptr)
{
    //static_assert(std::is_base_of<IReferencable, T>::value, "Type should be based on IReferencable");
    return NoAddRefWrapper((T*)ptr);
}

template< typename T >
INLINE static AddRefWrapper<T> AddRef(T* ptr)
{
    //static_assert(std::is_base_of<IReferencable, T>::value, "Type should be based on IReferencable");
    return AddRefWrapper<T>(ptr);
}

template< typename T >
INLINE static AddRefWrapper<T> AddRef(const T* ptr)
{
    //static_assert(std::is_base_of<IReferencable, T>::value, "Type should be based on IReferencable");
    return AddRefWrapper<T>((T*)ptr);
}

//--

static const int INVALID_ID = -1;

/// index of memory block
typedef void* MemoryBlock;

/// invalid memory block (unallocated)
static inline const MemoryBlock INVALID_MEMORY_BLOCK = nullptr;

//--

//----

END_BOOMER_NAMESPACE()



