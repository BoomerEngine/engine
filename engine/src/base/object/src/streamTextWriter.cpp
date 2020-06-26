/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\text #]
***/

#include "build.h"
#include "streamTextWriter.h"

namespace base
{
    namespace stream
    {

        ITextWriter::ITextWriter()
            : m_saveEditorOnlyProperties(false)
        {}

        ITextWriter::~ITextWriter()
        {
        }

    }
}

