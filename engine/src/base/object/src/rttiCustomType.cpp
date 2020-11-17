/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiCustomType.h"

#include "streamOpcodeWriter.h"
#include "streamOpcodeReader.h"

namespace base
{
    namespace rtti
    {

        //--

        CustomType::CustomType(const char* name, uint32_t size, uint32_t alignment, uint64_t nativeHash)
            : IType(StringID(name))
        {
            m_traits.nativeHash = nativeHash;
            m_traits.size = size;
            m_traits.alignment = alignment;
            m_traits.metaType = MetaType::Simple;
            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = false;
            m_traits.requiresDestructor = false;
            m_traits.simpleCopyCompare = true;
        }

        CustomType::~CustomType()
        {}

        void CustomType::construct(void* object) const
        {
            if (funcConstruct)
                funcConstruct(object);
            else
                memset(object, 0, m_traits.size);
        }

        void CustomType::destruct(void* object) const
        {
            if (funcDestruct)
                funcDestruct(object);
        }

        bool CustomType::compare(const void* data1, const void* data2) const
        {
            if (funcComare)
                return funcComare(data1, data2);
            else
                return 0 == memcmp(data1, data2, m_traits.size);
        }

        void CustomType::copy(void* dest, const void* src) const
        {
            if (funcCopy)
                return funcCopy(dest, src);
            else
                memcpy(dest, src, m_traits.size);
        }

        void CustomType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            if (funcWriteBinary)
            {
                funcWriteBinary(typeContext, file, data, defaultData);
            }
            else
            {
                TRACE_WARNING("Writing type '{}' at {} that has no binary serialization", name(), typeContext);
            }
        }

        void CustomType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            if (funcReadBinary)
            {
                funcReadBinary(typeContext, file, data);
            }
            else
            {
                TRACE_WARNING("Reading type '{}' at {} that has no binary serialization", name(), typeContext);
            }
        }

        void CustomType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
        {
            if (funcWriteXML)
            {
                funcWriteXML(typeContext, node, data, defaultData);
            }
            else
            {
                TRACE_WARNING("Writing type '{}' at {} that has no XML serialization", name(), typeContext);
            }
        }

        void CustomType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
        {
            if (funcReadXML)
            {
                funcReadXML(typeContext, node, data);
            }
            else
            {
                TRACE_WARNING("Reading type '{}' at {} that has no XML serialization", name(), typeContext);
            }
        }

        void CustomType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            if (funcPrintToText)
                funcPrintToText(f, data, flags);
        }

        bool CustomType::parseFromString(StringView txt, void* data, uint32_t flags) const
        {
            if (funcParseFromText)
                funcParseFromText(txt, data, flags);
            return false;
        }

        DataViewResult CustomType::describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (funcDescribeView)
                return funcDescribeView(viewPath, viewData, outInfo);

            return IType::describeDataView(viewPath, viewData, outInfo);
        }

        DataViewResult CustomType::readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (funcReadDataView)
                return funcReadDataView(viewPath, viewData, targetData, targetType);

            return IType::readDataView(viewPath, viewData, targetData, targetType);
        }

        DataViewResult CustomType::writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (funcWriteDataView)
                return funcWriteDataView(viewPath, viewData, sourceData, sourceType);

            return IType::writeDataView(viewPath, viewData, sourceData, sourceType);
        }

        //--

    } // rtti
} // base