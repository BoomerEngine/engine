/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "stringFileWriter.h"
#include "fileHandle.h"

BEGIN_BOOMER_NAMESPACE()

StringFileWriter::StringFileWriter(IWriteFileHandle* file)
    : m_file(AddRef(file))
{}

StringFileWriter::~StringFileWriter()
{
    flush();
}

void StringFileWriter::discardContent()
{
    if (m_file)
    {
        m_file->discardContent();
        m_file.reset();
    }

    m_errors = true;
}

void StringFileWriter::flush()
{
    StringWriter::flush();

    /*if (m_file)
        m_file->flush();*/
}

void StringFileWriter::writeData(const void* data, uint32_t size)
{
    if (!m_errors && m_file && size)
    {
        const auto numWritten = m_file->writeSync(data, size);
        if (numWritten != size)
        {
            TRACE_ERROR("String file writer failed to write {} bytes, got only {} written", size, numWritten);
            m_errors = true;

            discardContent();
        }
    }
}

END_BOOMER_NAMESPACE()
