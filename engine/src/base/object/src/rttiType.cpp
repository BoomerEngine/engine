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

        bool IType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (viewPath.empty())
            {
                outInfo.dataPtr = viewData;
                outInfo.dataType = this; // that's all we know :D

                if (outInfo.requestFlags.test(DataViewRequestFlagBit::TypeMetadata))
                    collectMetadataList(outInfo.metadata);

                outInfo.flags |= DataViewInfoFlagBit::LikeValue;
                return true;
            }

            return false;
        }

        bool IType::readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (viewPath.empty())
            {
                // general comparison vs base property
                if (targetType == DataViewBaseValue::GetStaticClass())
                {
                    auto& ret = *(DataViewBaseValue*)targetData;

                    if (rootView)
                    {
                        if (auto baseView = rootView->base())
                        {
                            ret.differentThanBase = !IDataView::Compare(*rootView, *baseView, rootViewPath);
                        }
                    }

                    return true;
                }

                return rtti::ConvertData(viewData, this, targetData, targetType);
            }

            return false;
        }

        bool IType::writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (viewPath.empty())
            {
                // reset command
                if (sourceType == DataViewCommand::GetStaticClass())
                {
                    const auto& cmd = *(const DataViewCommand*)sourceData;

                    if (cmd.command == "reset"_id)
                    {
                        if (rootView)
                        {
                            if (auto baseView = rootView->base())
                            {
                                return IDataView::Copy(*baseView, *rootView, rootViewPath);
                            }
                        }
                    }

                    return false;
                }

                return rtti::ConvertData(sourceData, sourceType, viewData, this);
            }
            return false;
        }

        ///---

    } // rtti
} // base