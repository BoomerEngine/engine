/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptPortableStubs.h"
#include "scriptPortableData.h"
#include "scriptPortableSerialization.h"

namespace base
{
    namespace script
    {

        //--
#define OPCODE(x) RTTI_ENUM_OPTION(x)
        RTTI_BEGIN_TYPE_ENUM(Opcode);
#include "scriptOpcodesRaw.h"//;
        RTTI_END_TYPE();
#undef OPCODE

        bool FindOpcodeByName(StringID name, Opcode& outOpcode)
        {
            static auto opcodeEnumType  = static_cast<const rtti::EnumType*>(base::reflection::GetTypeObject<Opcode>().ptr());
            int64_t value = 0;
            if (!opcodeEnumType->findValue(name, value))
                return false;
            outOpcode = (Opcode)value;
            return true;
        }

        //--

        RTTI_BEGIN_TYPE_CLASS(PortableData);
            RTTI_PROPERTY(m_packedData);
        RTTI_END_TYPE();

        PortableData::PortableData()
            : m_unpackMemory(POOL_SCRIPTS)
            , m_exportModule(nullptr)
        {}

        void PortableData::onPostLoad()
        {
            TBaseClass::onPostLoad();
            unpackData();
        }

        void PortableData::unpackData()
        {
            StubDataReader reader(m_packedData.data(), m_packedData.size(), m_unpackMemory);

            reader.readContainers();
            reader.allStubs(m_stubs);

            m_exportModule = reader.exportedModule();
        }

        //--

        RefPtr<PortableData> PortableData::Create(const StubModule* rootExportModule)
        {
            // map object structure, use the export module and anything it's using
            StubMapper mapper;
            mapper.processObject(rootExportModule);
            while (mapper.processRemainingObjects()) {}; // map everything, may require few passes

            // list captured stuff
            TRACE_INFO("Found {} stubs, {} names, {} strings", mapper.m_stubs.size(), mapper.m_names.size(), mapper.m_strings.size());

            // write data to memory buffer
            PagedBuffer memoryWriter(POOL_SCRIPTS);
            StubDataWriter stubWriter(mapper, memoryWriter);
            stubWriter.writeContainers();

            // report size of data
            TRACE_INFO("Packed script data size: {}", MemSize(memoryWriter.dataSize()));

            // create output object
            auto ret = RefNew<PortableData>();
            ret->m_packedData = memoryWriter.toBuffer();

            // load data again (makes copy)
            ret->unpackData();
            return ret;
        }

        //--

    } // script
} // base