/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"
#include "scriptFileParserHelper.h"
#include "scriptFunctionCode.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

class IErrorHandler;

/// parse content of function
class FunctionParser : public NoCopy
{
public:
    FunctionParser(LinearAllocator& mem, IErrorHandler& err, StubLibrary& library);
    ~FunctionParser();

    //--

    /// get shared stub library
    INLINE StubLibrary& stubs() const { return m_stubs; }

    /// get error reporter
    INLINE IErrorHandler& errorHandler() const { return m_err; }

    //--

    // process code of a function
    bool processCode(const StubFunction* functionStub, FunctionCode& outCode);

private:
    LinearAllocator& m_mem;
    IErrorHandler& m_err;
    StubLibrary& m_stubs;
};

//---

END_BOOMER_NAMESPACE_EX(script)
