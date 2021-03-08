/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "core/containers/include/stringWriter.h"

BEGIN_BOOMER_NAMESPACE()

//--

// a file based string writer
class CORE_IO_API StringFileWriter : public StringWriter
{
public:
    StringFileWriter(IWriteFileHandle* file);
    virtual ~StringFileWriter();

    INLINE bool hasErrors() const { return m_errors; }

    void discardContent();

    virtual void flush() override;

protected:
    virtual void writeData(const void* data, uint32_t size) override final;

    WriteFileHandlePtr m_file;
    bool m_errors = false;
};

///--

END_BOOMER_NAMESPACE()
