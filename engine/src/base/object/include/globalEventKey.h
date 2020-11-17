/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#ifdef BUILD_DEBUG
    #define GLOBAL_EVENTS_DEBUG_INFO
#endif

namespace base
{
    //--

    typedef uint64_t GlobalEventKeyType;

    enum EGlobalKeyAutoInit { GLOBAL_KEY_INIT };

    /// Event key in the global event system, it's just an unique number
    /// I assume that if you register an event listener you have access to the target, thus you can query this ID
    struct BASE_OBJECT_API GlobalEventKey
    {
    public:
        ALWAYS_INLINE GlobalEventKey() = default; // empty ID
        ALWAYS_INLINE GlobalEventKey(EGlobalKeyAutoInit); // auto initialize
        ALWAYS_INLINE GlobalEventKey(const GlobalEventKey& other) = default;
        ALWAYS_INLINE GlobalEventKey& operator=(const GlobalEventKey& other) = default;
        ALWAYS_INLINE GlobalEventKey(GlobalEventKey&& other);
        ALWAYS_INLINE GlobalEventKey& operator=(GlobalEventKey&& other);
        ALWAYS_INLINE ~GlobalEventKey() = default; // TODO: consider reuse

        ALWAYS_INLINE operator bool() const;
        ALWAYS_INLINE bool valid() const;

        ALWAYS_INLINE void reset();

        ALWAYS_INLINE GlobalEventKeyType rawValue() const;

        ALWAYS_INLINE bool operator==(const GlobalEventKey& other) const;
        ALWAYS_INLINE bool operator!=(const GlobalEventKey& other) const;
        ALWAYS_INLINE bool operator<(const GlobalEventKey& other) const;

        ALWAYS_INLINE static uint32_t CalcHash(const GlobalEventKey& key);

        void print(IFormatStream& f) const;

    private:
        uint64_t m_key = 0; // TODO: measure usage and do 128 bit...

#ifdef GLOBAL_EVENTS_DEBUG_INFO
        StringBuf m_debugInfo;
#endif

        friend BASE_OBJECT_API GlobalEventKey MakeUniqueEventKey(StringView);
        friend BASE_OBJECT_API GlobalEventKey MakeSharedEventKey(StringView);
    };

    //--

    // allocate unique event key, good for tracking object instances, etc
    extern BASE_OBJECT_API GlobalEventKey MakeUniqueEventKey(StringView debugInfo = "");

    // allocate shared event key - will return same value for the same path, great for files
    extern BASE_OBJECT_API GlobalEventKey MakeSharedEventKey(StringView path);

    //--

} // base

#include "globalEventKey.inl"
