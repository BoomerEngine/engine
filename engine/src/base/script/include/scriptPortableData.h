/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "scriptOpcodes.h"
#include "base/memory/include/linearAllocator.h"
#include "base/object/include/object.h"

namespace base
{
    namespace script
    {

        ///---

        struct Stub;
        struct StubModule;
        struct StubFunction;
        struct StubClass;
        struct StubEnum;
        struct StubTypeDecl;

        ///---

        /// serialized portable compiled script module
        /// contains all type definitions, stubs, and functions with portable opcodes
        class BASE_SCRIPT_API PortableData : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(PortableData, IObject);

        public:
            PortableData();

            //--

            // get all the stubs
            INLINE const Array<const Stub*>& allStubs() const { return m_stubs; }

            // get the export module
            INLINE const StubModule* exportedModule() const { return m_exportModule; }

            //--

            // create new data
            static RefPtr<PortableData> Create(const StubModule* rootExportModule);

        private:
            Buffer  m_packedData; // packed data
            mem::LinearAllocator m_unpackMemory; // memory for all of the unpacked data

            Array<const Stub*> m_stubs; // all stubs in the
            const StubModule* m_exportModule; // exported module (contains all data actually defined in the script package)

            //--

            // IObject
            virtual void onPostLoad() override final;

            void unpackData();
        };

        ///---

    } // script
} // base

