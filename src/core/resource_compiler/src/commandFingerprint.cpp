/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "commandFingerprint.h"
#include "importFileFingerprintService.h"
#include "importFileService.h"

#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/io/include/ioSystem.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--


RTTI_BEGIN_TYPE_CLASS(CommandFingerprint);
RTTI_METADATA(app::CommandNameMetadata).name("fingerprint");
RTTI_END_TYPE();

//--

bool CommandFingerprint::run(IProgressTracker* progress, const app::CommandLine& commandline)
{
    // find the source asset service - we need it to have access to source assets
    auto fingerprintService = GetService<ImportFileFingerprintService>();
    if (!fingerprintService)
    {
        TRACE_ERROR("Source fingerprint service not started, importing not possible");
        return false;
    }

    // absolute path ?
    if (const auto absolutePath = commandline.singleValue("absolutePath"))
    {
        // open source file
        if (auto file = io::OpenForAsyncReading(absolutePath))
        {
            ScopeTimer timer;
            ImportFileFingerprint fingerprint;
            const auto ret = CalculateFileFingerprint(file, true, nullptr, fingerprint);

            if (ret == FingerpintCalculationStatus::Canceled)
            {
                TRACE_ERROR("Fingerprint result: Operation canceled");
                return false;
            }
            else if (ret == FingerpintCalculationStatus::ErrorNoFile)
            {
                TRACE_ERROR("Fingerprint result: No file");
                return false;
            }
            else if (ret == FingerpintCalculationStatus::ErrorInvalidRead)
            {
                TRACE_ERROR("Fingerprint result: Invalid read");
                return false;
            }
            else if (ret == FingerpintCalculationStatus::ErrorOutOfMemory)
            {
                TRACE_ERROR("Fingerprint result: Out of memory");
                return false;
            }
            else
            {
                TRACE_WARNING("Calculated fingerprint for '{}': {} ({})", absolutePath, fingerprint, timer);
            }
        }
        else
        {
            TRACE_ERROR("Unable to open '{}'", absolutePath);
            return false;
        }

    }
    else if (const auto assetPath = commandline.singleValue("assetPath"))
    {
        // find the source asset service - we need it to have access to source assets
        auto assetSource = GetService<ImportFileService>();
        if (!assetSource)
        {
            TRACE_ERROR("Source fingerprint service not started, importing not possible");
            return false;
        }

        // open the asset reader
        //assetSource->
    }

    // we are done
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(res)
