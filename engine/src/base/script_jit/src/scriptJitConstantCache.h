/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/script/include/scriptPortableData.h"
#include "base/script/include/scriptPortableStubs.h"
#include "base/script/include/scriptJIT.h"

namespace base
{
    namespace script
    {

        //--

        /// JIT const cache
        class BASE_SCRIPT_JIT_API JITConstCache : public NoCopy
        {
        public:
            JITConstCache(mem::LinearAllocator& mem, JITTypeLib& typeLib);
            ~JITConstCache();

            /// print constant initialization vars
            void printConstVars(IFormatStream& f) const;

            /// print constant initialization code
            void printConstInit(IFormatStream& f) const;

            //--

            /// map a string constant
            StringView mapStringConst(const char* stringConst);

            /// map a name constant
            StringView mapNameConst(const char* nameConst);

            /// map a type constant
            StringView mapTypeConst(const char* typeConst);

        private:
            mem::LinearAllocator& m_mem;
            JITTypeLib& m_typeLib;

            const JITType* m_stringType = nullptr;
            const JITType* m_nameType = nullptr;
            const JITType* m_typeType = nullptr;

            struct Constant
            {
                StringView m_varName;
                const char* m_string;
            };

            Array<Constant> m_stringConstants;
            Array<Constant> m_nameConstants;
            Array<Constant> m_typeConstants;

            HashMap<uint64_t, uint32_t> m_stringConstantMap;
            HashMap<uint64_t, uint32_t> m_nameConstantMap;
            HashMap<uint64_t, uint32_t> m_typeConstantMap;
        };

        //--

    } // script
} // base