/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\debug #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::debug)

//---

class BASE_SYSTEM_API SymbolName
{
public:
    static const uint32_t MAX_SIZE = 512;

    SymbolName();

    INLINE const char* c_str() const { return m_txt; }

    void set(const char* txt);
    void append(const char* txt);

private:
    char m_txt[MAX_SIZE + 1];
};

//---

class BASE_SYSTEM_API Callstack
{
public:
    static const uint32_t MAX_FRAMES = 64;

    Callstack();
            
    void reset();
    void push(uint64_t address);

    INLINE bool empty() const
    {
        return (m_size == 0);
    }

    INLINE uint32_t size() const
    {
        return m_size;
    }

    INLINE const uint64_t operator[](const uint32_t index) const
    {
        if (index < MAX_FRAMES)
            return m_frames[index];
        return 0;
    }

	void print(IFormatStream& f, const char* lineSearator = "\n") const;

    uint64_t uniqueHash() const;

private:
    uint32_t m_size;
    uint64_t m_frames[MAX_FRAMES];
};

//---

// Debug break function
extern BASE_SYSTEM_API void Break();

// Grab current callstack, returns number of function grabbed (0 if error)
extern BASE_SYSTEM_API bool GrabCallstack(uint32_t skipInitialFunctions, const void* exptr, Callstack& outCallstack);

// Get human readable function name
extern BASE_SYSTEM_API bool TranslateSymbolName(uint64_t functionAddress, SymbolName& outSymbolName, SymbolName& outFileName);

// Check if debugger is present (windows mostly)
extern BASE_SYSTEM_API bool DebuggerPresent();

//--

// Get indexed callstack (as an easy to save number)
extern BASE_SYSTEM_API uint32_t CaptureCallstack(uint32_t skipInitialFunctions);

// Resolve callstack by ID, can be used to print it
extern BASE_SYSTEM_API const Callstack& ResolveCapturedCallstack(uint32_t id);

//--

END_BOOMER_NAMESPACE(base::debug)