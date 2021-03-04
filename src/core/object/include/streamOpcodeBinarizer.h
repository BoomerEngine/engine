/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "core/containers/include/hashSet.h"

#include "streamOpcodes.h"
#include "streamOpcodeWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(stream)

//--

/// mapped tables for writing
struct CORE_OBJECT_API OpcodeMappedReferences : public NoCopy
{
    OpcodeMappedReferences();

    HashMap<StringID, uint32_t> mappedNames;
    HashMap<Type, uint32_t> mappedTypes;
    HashMap<const IObject*, uint32_t> mappedPointers;
    HashMap<const rtti::Property*, uint32_t> mappedProperties;
    HashMap<OpcodeWriterResourceReference, uint32_t> mappedResources;
};

//--

// binary data writer for the token stream, provides basic buffering and translation
class CORE_OBJECT_API OpcodeFileWriter : public NoCopy
{
public:
    OpcodeFileWriter(IWriteFileHandle* outputFile);
    ~OpcodeFileWriter();

    //--

    // get CRC of written data
    INLINE uint32_t crc() const { return m_crc.crc(); }

    //--

    // flush internal buffer
    void flush();

    // write given amount of data to file
    void writeToFile(const void* data, uint64_t size);

private:
    static const uint32_t BUFFER_SIZE = 4096;

    IWriteFileHandle* m_outputFile;

    uint8_t m_cacheBuffer[BUFFER_SIZE];
    uint32_t m_cacheBufferSize = 0;

    CRC32 m_crc;
};

//--

// write opcode stream to file
extern void CORE_OBJECT_API WriteOpcodes(bool protectedStream, const OpcodeStream& stream, const OpcodeMappedReferences& mappedReferences, OpcodeFileWriter& writer);
            
//--

END_BOOMER_NAMESPACE_EX(stream)
