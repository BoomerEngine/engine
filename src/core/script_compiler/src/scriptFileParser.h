/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

class StubLibrary;
class StubLibraryTypeCache;

/// parse content of script file
class FileParser : public NoCopy
{
public:
    FileParser(LinearAllocator& mem, IErrorHandler& err, StubLibrary& library);
    ~FileParser();

    //--

    /// get shared stub library
    INLINE StubLibrary& stubs() const { return m_library; }

    /// get error reporter
    INLINE IErrorHandler& errorHandler() const { return m_err; }

    //--

    // process code of a file, creates stubs
    bool processCode(const StubFile* fileStub, const Buffer& code);

private:
    LinearAllocator& m_mem;
    IErrorHandler& m_err;
    StubLibrary& m_library;
};

//---

END_BOOMER_NAMESPACE_EX(script)
