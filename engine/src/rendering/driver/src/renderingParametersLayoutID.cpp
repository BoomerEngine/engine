/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingParametersLayoutInfo.h"
#include "renderingParametersLayoutID.h"
#include "renderingConstantsView.h"
#include "renderingImageView.h"
#include "renderingBufferView.h"

#include "base/containers/include/stringParser.h"

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

            ParametersLayoutID registerEntry(const ParametersLayoutInfo& info)
            {
                // take the lock
                auto lock = base::CreateLock(m_lock);

                // compute the hash
                auto hash = info.hash();
                Entry* entry = nullptr;
                if (m_entriesMap.find(hash, entry))
                {
                    ASSERT(entry->layout == info);
                    return ParametersLayoutID(entry->id);
                }

                // add new entry
                entry = MemNew(Entry);
                entry->id = (uint16_t)m_entries.size();
                entry->layout = std::move(ParametersLayoutInfo(info, ParametersLayoutID(entry->id)));
                entry->text = base::TempString("{}", info);
                m_entriesMap[hash] = entry;
                m_entries.pushBack(entry);
                TRACE_INFO("Register parameter layout '{}' at ID {}", entry->text, entry->id);

                // return the ID of the new entry
                return ParametersLayoutID(entry->id);
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

            const ParametersLayoutInfo& entryLayout(uint16_t id) const
            {
                // take the lock
                auto lock = base::CreateLock(m_lock);

                // filter out invalid IDs
                if (id == 0 || id >= m_entries.size())
                {
                    static ParametersLayoutInfo theEmptyEntry;
                    return theEmptyEntry;
                }

                // return the cached layout
                return m_entries[id]->layout;
            }

        private:
            struct Entry
            {
                uint16_t id = 0;
                ParametersLayoutInfo layout;
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

    void ParametersLayoutID::print(base::IFormatStream& f) const
    {
        f << helper::LayoutRegistry::GetInstance().entryName(id);
    }

    const ParametersLayoutInfo& ParametersLayoutID::layout() const
    {
        return helper::LayoutRegistry::GetInstance().entryLayout(id);
    }

    uint32_t ParametersLayoutID::memorySize() const
    {
        return layout().memorySize();
    }

    ParametersLayoutID ParametersLayoutID::FromObjectTypes(const ObjectViewType* types, uint32_t numTypes)
    {
        if (!types || !numTypes)
            return ParametersLayoutID();

        // protect from to many vertices
        //DEBUG_CHECK_EX(numTypes <= 32, "Lots of resources in parameter layout, check for errors");

        // setup a layout object
        ParametersLayoutInfo info;
        info.m_entries.reserve(numTypes);

        for (uint32_t i=0; i<numTypes; ++i)
        {
            const auto type = types[i];

            info.m_memorySize = base::Align<uint32_t>(info.m_memorySize, 4);
            info.m_entries.pushBack(type);

            switch (type)
            {
            case ObjectViewType::Constants:
                info.m_memorySize += sizeof(ConstantsView);
                break;

            case ObjectViewType::Image:
                info.m_memorySize += sizeof(ImageView);
                break;

            case ObjectViewType::Buffer:
                info.m_memorySize += sizeof(BufferView);
                break;
            }
        }

        // just use the data hash as the key
        // TODO: we COULD bit pack the types into unique key but we are not guaranteed to only have <32 descriptors 
        info.m_hash = base::CRC64().append(types, numTypes).crc();

        // register the layout, return it's ID
        return ParametersLayoutID::Register(info);
    }

    ParametersLayoutID ParametersLayoutID::FromData(const void* data, uint32_t dataSize)
    {
        // nothing to build from
        if (!data || !dataSize)
            return ParametersLayoutID();

        base::InplaceArray<ObjectViewType, 32> viewTypes;

        // minimal needed size for data
        static const auto minimalSize = std::min<uint32_t>(sizeof(ConstantsView), std::min<uint32_t>(sizeof(BufferView), sizeof(ImageView)));

        // walk the entries
        auto readPtr  = (const uint8_t*)data;
        auto endPtr  = readPtr + dataSize - (minimalSize-1);
        while (readPtr < endPtr)
        {
            auto type = (ObjectViewType)*readPtr;
            switch (type)
            {
                case ObjectViewType::Constants:
                {
                    auto view  = (const ConstantsView*)readPtr;
                    readPtr += sizeof(ConstantsView);
                    viewTypes.pushBack(ObjectViewType::Constants);
                    break;
                }

                case ObjectViewType::Buffer:
                {
                    auto view  = (const BufferView*)readPtr;
                    readPtr += sizeof(BufferView);
                    viewTypes.pushBack(ObjectViewType::Buffer);
                    break;
                }

                case ObjectViewType::Image:
                {
                    auto view  = (const ImageView*)readPtr;
                    readPtr += sizeof(ImageView);
                    viewTypes.pushBack(ObjectViewType::Image);
                    break;
                }

                default:
                {
                    TRACE_ERROR("Unable to build valid ParametersLayout from provided structure, unknown view object {}", (int)type);
                    DEBUG_CHECK(!"Unable to build valid ParametersLayout from provided structure");
                    return ParametersLayoutID();
                }
            }
        }

        return FromObjectTypes(viewTypes.typedData(), viewTypes.size());
    }

    ParametersLayoutID ParametersLayoutID::Register(const ParametersLayoutInfo& info)
    {
        // empty
        if (info.empty())
            return ParametersLayoutID();

        // register in global registry
        return helper::LayoutRegistry::GetInstance().registerEntry(info);
    }

    ParametersLayoutID ParametersLayoutID::Register(base::StringView layoutDesc)
    {
        // empty
        if (!layoutDesc)
            return ParametersLayoutID();

        // parse from string
        base::InplaceArray<ObjectViewType, 32> viewTypes;
        for (const auto ch : layoutDesc)
        {
            switch (ch)
            {
            case 'I':
                viewTypes.pushBack(ObjectViewType::Image);
                break;

            case 'B':
                viewTypes.pushBack(ObjectViewType::Buffer);
                break;

            case 'C':
                viewTypes.pushBack(ObjectViewType::Constants);
                break;

            default:
                DEBUG_CHECK_EX(false, base::TempString("Unrecognized resource type: {}", ch));
                break;
            }
        }

        // return the ID of registered layout
        return FromObjectTypes(viewTypes.typedData(), viewTypes.size());
    }

} // rendering