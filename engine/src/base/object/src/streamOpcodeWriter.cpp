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
#include "streamOpcodes.h"
#include "streamOpcodeWriter.h"

namespace base
{
    namespace stream
    {
        //--

        OpcodeWriterReferences::OpcodeWriterReferences()
        {
            stringIds.reserve(8);
            objects.reserve(8);
            properties.reserve(8);
            syncResources.reserve(8);
            asyncResources.reserve(8);
        }
        
        //--

        OpcodeWriter::OpcodeWriter(OpcodeStream& stream, OpcodeWriterReferences& referenceCollector)
            : m_references(referenceCollector)
            , m_stream(stream)
        {
        }

        OpcodeWriter::~OpcodeWriter()
        {
        }       
    

        void OpcodeWriter::writeResourceReference(StringView<char> path, ClassType resourceClass, bool async)
        {
            if (path && resourceClass)
            {
                OpcodeWriterResourceReference resref;
                resref.resourcePath = StringBuf(path);
                resref.resourceType = resourceClass;

                if (async)
                    m_references.asyncResources.insert(resref);
                else
                    m_references.syncResources.insert(resref);

                if (auto op = m_stream.allocOpcode<StreamOpDataResourceRef>())
                {
                    op->path = resref.resourcePath;
                    op->type = resref.resourceType;
                    op->async = async;

                    m_stream.m_resourceReferences.pushBack(op);
                }
            }
            else
            {
                m_stream.allocOpcode<StreamOpDataResourceRef>();
            }
        }

        void OpcodeWriter::writeBuffer(const Buffer& buffer)
        {
            if (auto op = m_stream.allocOpcode<StreamOpDataInlineBuffer>())
            {
                op->buffer = buffer;
                m_stream.m_inlinedBuffers.pushBack(op);
                m_stream.m_totalDataSize += buffer.size();
            }
        }

        //--

    } // stream
} // base
