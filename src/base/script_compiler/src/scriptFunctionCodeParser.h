/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "scriptFileParserHelper.h"
#include "scriptFunctionCode.h"

namespace base
{
    namespace script
    {

        //---

        class IErrorHandler;

        /// parse content of function
        class FunctionParser : public base::NoCopy
        {
        public:
            FunctionParser(mem::LinearAllocator& mem, IErrorHandler& err, StubLibrary& library);
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
            mem::LinearAllocator& m_mem;
            IErrorHandler& m_err;
            StubLibrary& m_stubs;
        };

        //---

    } // script
} // base