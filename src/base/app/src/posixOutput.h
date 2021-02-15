/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\posix #]
* [# platform: posix #]
***/

#pragma once

#include "base/system/include/output.h"

namespace base
{
    namespace platform
    {
        namespace posix
        {
            // WinAPI console log output
            class GenericOutput : public log::ILogSink, public log::IErrorHandler
            {
            public:
                GenericOutput();
                ~GenericOutput();

            private:
                // IOutputListener interface
                virtual void writeLine(const log::OutputLevel level, const char* text)  override final;

                // IErrorListener interface
                virtual void handleFatalError(const char* fileName, uint32_t fileLine, const char* txt) override final;
                virtual void handleAssert(bool isFatal, const char* fileName, uint32_t fileLine, const char* expr, const char* msg, bool* isEnabled) override final;
            };

        } // posix
    } // platform
} //  base