/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiDataView.h"
#include "streamBinaryWriter.h"
#include "rttiDataHolder.h"
#include "dataView.h"

#include "base/containers/include/stringBuilder.h"

namespace base
{
    namespace rtti
    {

        ///---

        IType::IType(StringID name)
            : m_name(name)
        {}

        IType::~IType()
        {}

        void IType::cacheTypeData()
        {}

        void IType::releaseTypeReferences()
        {}

        void IType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            // nothing
        }

        bool IType::parseFromString(StringView<char> txt, void* data, uint32_t flags) const
        {
            // unparsable
            return false;
        }

        class CRC64Writer : public stream::IBinaryWriter
        {
        public:
            CRC64Writer(CRC64& crc)
                : IBinaryWriter((uint64_t)stream::BinaryStreamFlags::MemoryBased)
                , m_crc(crc)
            {}

            virtual void write(const void* data, uint32_t size) override final
            {
                m_crc.append(data, size);
                m_pos += size;
                m_size = std::max<uint32_t>(m_pos, m_size);
            }

            virtual uint64_t pos() const override final { return m_pos; }
            virtual uint64_t size() const override final { return m_size; }
            virtual void seek(uint64_t pos) override final { m_pos = pos; }

        private:
            CRC64& m_crc;

            uint64_t m_size = 0;
            uint64_t m_pos = 0;
        };

        void IType::calcCRC64(CRC64& crc, const void* data) const
        {
            CRC64Writer writer(crc);
            writeBinary(TypeSerializationContext(), writer, data, nullptr);
        }

        //--

        DataViewResult IType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (viewPath.empty())
            {
                outInfo.dataPtr = viewData;
                outInfo.dataType = this; // that's all we know :D

                if (outInfo.requestFlags.test(DataViewRequestFlagBit::TypeMetadata))
                    collectMetadataList(outInfo.metadata);

                outInfo.flags |= DataViewInfoFlagBit::LikeValue;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
                return DataViewResultCode::ErrorUnknownProperty;

            return DataViewResultCode::ErrorIllegalAccess;
        }

        DataViewResult IType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (viewPath.empty())
            {
                if (!rtti::ConvertData(viewData, this, targetData, targetType))
                    return DataViewResultCode::ErrorTypeConversion;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                if (propertyName == "__type")
                {
                     // TODO
                }

                return DataViewResultCode::ErrorUnknownProperty;
            }

            return DataViewResultCode::ErrorIllegalAccess;
        }

        DataViewResult IType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (viewPath.empty())
            {
                if (!rtti::ConvertData(sourceData, sourceType, viewData, this))
                    return DataViewResultCode::ErrorTypeConversion;
                return DataViewResultCode::OK;
            }

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                return DataViewResultCode::ErrorUnknownProperty;
            }

            return DataViewResultCode::ErrorIllegalAccess;
        }

        ///---

    } // rtti
} // base