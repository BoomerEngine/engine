/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiProperty.h"

#include "asyncBuffer.h"
#include "serializationStream.h"
#include "serializationWriter.h"

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("",off)

//--

SerializationWriterReferences::SerializationWriterReferences()
{
    stringIds.reserve(8);
    objects.reserve(8);
    properties.reserve(8);
    syncResources.reserve(8);
    asyncResources.reserve(8);
}
        
//--

SerializationWriter::SerializationWriter(SerializationStream& stream, SerializationWriterReferences& referenceCollector)
    : m_references(referenceCollector)
    , m_stream(stream)
{
}

SerializationWriter::~SerializationWriter()
{
}       
    

void SerializationWriter::writeResourceReference(GUID id, ClassType resourceClass, bool async)
{
    if (id && resourceClass)
    {
        SerializationWriterResourceReference resref;
        resref.resourceID = id;
        resref.resourceType = resourceClass;

        if (async)
            m_references.asyncResources.insert(resref);
        else
            m_references.syncResources.insert(resref);

        if (auto op = m_stream.allocOpcode<SerializationOpDataResourceRef>())
        {
            op->id = id;
            op->type = resref.resourceType;
            op->async = async;

            m_stream.m_resourceReferences.pushBack(op);
        }
    }
    else
    {
        m_stream.allocOpcode<SerializationOpDataResourceRef>();
    }
}

void SerializationWriter::writeBuffer(Buffer data, CompressionType ct, uint64_t size, uint64_t crc)
{
    if (auto op = m_stream.allocOpcode<SerializationOpDataInlineBuffer>())
    {
        op->data = data;
        op->compressionType = ct;
        op->uncompressedSize = size;
        op->uncompressedCRC = crc;

        m_stream.m_inlinedBuffers.pushBack(op);
        m_stream.m_totalDataSize += data.size();
    }
}

void SerializationWriter::writeAsyncBuffer(IAsyncFileBufferLoader* loader)
{
    if (auto op = m_stream.allocOpcode<SerializationOpDataInlineBuffer>())
    {
        if (loader)
        {
            op->asyncLoader = AddRef(loader);
            op->compressionType = CompressionType::Uncompressed; // HACK: we don't have access to compression type without an "extract" call
            op->uncompressedSize = loader->size();
            op->uncompressedCRC = loader->crc();

            m_references.asyncBuffers.insert(AddRef(loader));
        }

        m_stream.m_totalDataSize += op->uncompressedSize;
        m_stream.m_inlinedBuffers.pushBack(op);
    }
}

//--

END_BOOMER_NAMESPACE()
