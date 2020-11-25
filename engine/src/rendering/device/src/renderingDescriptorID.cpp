/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#include "build.h"
#include "renderingDescriptorInfo.h"
#include "renderingDescriptorID.h"

#include "base/containers/include/stringParser.h"
#include "renderingDescriptor.h"

namespace rendering
{

    namespace helper
    {

        /// the registry of all of the layouts
        class LayoutRegistry : public base::ISingleton
        {
            DECLARE_SINGLETON(LayoutRegistry);

        public:
            LayoutRegistry()
            {
                m_entriesMap.reserve(256);
                m_entries.reserve(256);
                m_entries.pushBack(nullptr);
            }

            DescriptorID registerEntry(const DescriptorInfo& info)
            {
                // take the lock
                auto lock = base::CreateLock(m_lock);

                // compute the hash
                auto hash = info.hash();
                Entry* entry = nullptr;
                if (m_entriesMap.find(hash, entry))
                {
                    ASSERT(entry->layout == info);
                    return DescriptorID(entry->id);
                }

                // add new entry
                entry = new Entry;
                entry->id = (uint16_t)m_entries.size();
                entry->layout = std::move(DescriptorInfo(info, DescriptorID(entry->id)));
                entry->text = base::TempString("{}", info);
                m_entriesMap[hash] = entry;
                m_entries.pushBack(entry);
                TRACE_INFO("Register parameter layout '{}' at ID {}", entry->text, entry->id);

                // return the ID of the new entry
                return DescriptorID(entry->id);
            }

            base::StringBuf entryName(uint16_t id) const
            {
                // take the lock
                auto lock = base::CreateLock(m_lock);

                // filter out invalid IDs
                if (id == 0 || id >= m_entries.size())
                    return base::StringBuf("INVALID");

                // return the cached entry name
                return m_entries[id]->text;
            }

            const DescriptorInfo& entryLayout(uint16_t id) const
            {
                // take the lock
                auto lock = base::CreateLock(m_lock);

                // filter out invalid IDs
                if (id == 0 || id >= m_entries.size())
                {
                    static DescriptorInfo theEmptyEntry;
                    return theEmptyEntry;
                }

                // return the cached layout
                return m_entries[id]->layout;
            }

        private:
            struct Entry : public base::NoCopy
            {
                RTTI_DECLARE_POOL(POOL_RENDERING_PARAM_LAYOUT)

            public:
                uint16_t id = 0;
                DescriptorInfo layout;
                base::StringBuf text;
            };

            base::HashMap<uint64_t, Entry*> m_entriesMap;
            base::Array<Entry*> m_entries;

            mutable base::SpinLock m_lock;

            virtual void deinit() override
            {
                m_entries.clearPtr();
                m_entriesMap.clear();
            }
        };


    } // helper

    //--

    void DescriptorID::print(base::IFormatStream& f) const
    {
        f << helper::LayoutRegistry::GetInstance().entryName(id);
    }

    const DescriptorInfo& DescriptorID::layout() const
    {
        return helper::LayoutRegistry::GetInstance().entryLayout(id);
    }

    uint32_t DescriptorID::memorySize() const
    {
        return layout().size() * sizeof(DescriptorEntry);
    }

    DescriptorID DescriptorID::FromTypes(const DeviceObjectViewType* types, uint32_t numTypes)
    {
        DEBUG_CHECK_RETURN_V(types, DescriptorID());
        DEBUG_CHECK_RETURN_V(numTypes, DescriptorID());

        DescriptorInfo info;
        info.m_entries.reserve(numTypes);

        for (uint32_t i = 0; i < numTypes; ++i)
        {
            const auto type = types[i];
            DEBUG_CHECK_RETURN_V(type != DeviceObjectViewType::Invalid, DescriptorID());

            info.m_entries.pushBack(type);
        }

        // just use the data hash as the key
        // TODO: we COULD bit pack the types into unique key but we are not guaranteed to only have <32 descriptors 
        info.m_hash = base::CRC64().append(info.m_entries.data(), info.m_entries.dataSize()).crc();

        // register the layout, return it's ID
        return DescriptorID::Register(info);
    }

    DescriptorID DescriptorID::FromDescriptor(const DescriptorEntry* data, uint32_t count)
    {
        DEBUG_CHECK_RETURN_V(data, DescriptorID());
        DEBUG_CHECK_RETURN_V(count, DescriptorID());

        DescriptorInfo info;
        info.m_entries.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            const auto type = data[i].type;
            DEBUG_CHECK_RETURN_V(type != DeviceObjectViewType::Invalid, DescriptorID());

            info.m_entries.pushBack(type);
        }

        // just use the data hash as the key
        // TODO: we COULD bit pack the types into unique key but we are not guaranteed to only have <32 descriptors 
        info.m_hash = base::CRC64().append(info.m_entries.data(), info.m_entries.dataSize()).crc();

        // register the layout, return it's ID
        return DescriptorID::Register(info);
    }

    DescriptorID DescriptorID::Register(const DescriptorInfo& info)
    {
        // empty
        if (info.empty())
            return DescriptorID();

        // register in global registry
        return helper::LayoutRegistry::GetInstance().registerEntry(info);
    }

    DescriptorID DescriptorID::Register(base::StringView layoutDesc)
    {
        DEBUG_CHECK_RETURN_V(layoutDesc, DescriptorID());

        base::InplaceArray<DeviceObjectViewType, 32> viewTypes;
        base::StringParser parser(layoutDesc);

        bool first = true;

        while (!parser.parseWhitespaces())
        {
            // separator
            if (!first)
                DEBUG_CHECK_RETURN_V(parser.parseKeyword("-"), DescriptorID());
            first = false;

            // match type
            if (parser.parseKeyword("CBV"))
                viewTypes.pushBack(DeviceObjectViewType::ConstantBuffer);
            else if (parser.parseKeyword("BSRV"))
                viewTypes.pushBack(DeviceObjectViewType::Buffer);
            else if (parser.parseKeyword("BUAV"))
                viewTypes.pushBack(DeviceObjectViewType::BufferWritable);
            else if (parser.parseKeyword("ISRV"))
                viewTypes.pushBack(DeviceObjectViewType::Image);
            else if (parser.parseKeyword("IUAV"))
                viewTypes.pushBack(DeviceObjectViewType::ImageWritable);
            else if (parser.parseKeyword("SBSRV"))
                viewTypes.pushBack(DeviceObjectViewType::BufferStructured);
            else if (parser.parseKeyword("SBUAV"))
                viewTypes.pushBack(DeviceObjectViewType::BufferStructuredWritable);
            else if (parser.parseKeyword("RTV"))
                viewTypes.pushBack(DeviceObjectViewType::RenderTarget);
            else if (parser.parseKeyword("SMPL"))
                viewTypes.pushBack(DeviceObjectViewType::Sampler);
            else
            {
                DEBUG_CHECK(!"Invalid descriptor text");
                return DescriptorID();
            }
        }

        return FromTypes(viewTypes.typedData(), viewTypes.size());
    }

} // rendering