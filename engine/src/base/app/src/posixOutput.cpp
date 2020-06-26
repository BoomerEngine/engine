/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\posix #]
* [# platform: posix #]
***/

#include "build.h"

#include "posixOutput.h"

#include "base/system/include/debug.h"
#include "base/containers/include/stringBuilder.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[1;31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[1;33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KFATA "\x1B[1;30;41m"
#define RESET "\x1B[0m"

namespace base
{
    namespace platform
    {
        namespace posix
        {

            GenericOutput::GenericOutput()
            {}

            GenericOutput::~GenericOutput()
            {}

            void GenericOutput::writeLine(const log::OutputLevel level, const char* text)
            {
                if (level == log::OutputLevel::Spam)
                {
                    fprintf(stdout, RESET "%s" RESET, text);
                }
                else if (level == log::OutputLevel::Info)
                {
                    fprintf(stdout, KWHT "%s" RESET, text);
                }
                else if (level == log::OutputLevel::Warning)
                {
                    fprintf(stdout, KYEL "%s" RESET, text);
                }
                else if (level == log::OutputLevel::Error)
                {
                    fprintf(stderr, KRED "%s" RESET, text);
                }
                else if (level == log::OutputLevel::Fatal)
                {
                    fprintf(stderr, KRED "%s" RESET, text);
                }
            }

            void GenericOutput::handleFatalError(const char* fileName, uint32_t fileLine, const char* txt)
            {
                fprintf(stderr, KFATA "Fatal Error occurred in Boomer Engine\n" RESET);
                if (fileName && *fileName)
                    fprintf(stderr, KFATA "%s(%u): %s\n" RESET, fileName, fileLine, txt);
                debug::Break();
            }

            void GenericOutput::handleAssert(bool isFatal, const char* fileName, uint32_t fileLine, const char* expr, const char* msg, bool* isEnabled)
            {
                if (!isFatal)
                {
                    fprintf(stderr, KRED "Assertion failed in Boomer Engine:\n" RESET);

                    if (msg && *msg)
                        fprintf(stderr, KRED "%s(%u): %s\n" RESET, fileName, fileLine, msg);
                    else
                        fprintf(stderr, KRED "%s(%u): %s\n" RESET, fileName, fileLine, expr);
                }
                else
                {
                    fprintf(stderr, KFATA "Fatal Assertion failed in Boomer Engine\n" RESET);

                    if (msg && *msg)
                        fprintf(stderr, KFATA "%s(%u): %s\n" RESET, fileName, fileLine, msg);
                    else
                        fprintf(stderr, KFATA "%s(%u): %s\n" RESET, fileName, fileLine, expr);


                    debug::Break();
                }

                // disable assertion
                if (isEnabled)
                    *isEnabled = false;
            }

        } // posix
    } // platform
} // base