/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_script_compiler_glue.inl"

namespace base
{
    namespace script
    {

        /// error handler
        class BASE_SCRIPT_COMPILER_API IErrorHandler : public base::NoCopy
        {
        public:
            virtual ~IErrorHandler();

            /// report error
            virtual void reportError(const StringBuf& fullPath, uint32_t line, StringView message) = 0;

            /// report warning
            virtual void reportWarning(const StringBuf& fullPath, uint32_t line, StringView message) = 0;
        };

    } // script
} // base
