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

        /// piece of code emited by function JIT
        struct BASE_SCRIPT_JIT_API JITCodeChunkC
        {
            const JITType* jitType = nullptr;
            bool pointer = false;
            StringView<char> text;
            const StubOpcode* op = nullptr;

            INLINE void print(IFormatStream& f) const
            {
                f << text;
            }
        };

        //--

        /// opcode stream
        class BASE_SCRIPT_JIT_API JITOpcodeStream : public NoCopy
        {
        public:
            JITOpcodeStream(const StubFunction* func);

            /// peek next opcode
            const StubOpcode* peek() const;

            /// read next opcode
            const StubOpcode* read();

            /// has data ?
            INLINE operator bool() const { return m_pos < m_end; }

        private:
            uint32_t m_pos;
            uint32_t m_end;
            const StubFunction* m_func;
        };

        //--

        /// function code transformer
        class BASE_SCRIPT_JIT_API JITFunctionWriterC : public NoCopy
        {
        public:
            JITFunctionWriterC(mem::LinearAllocator& mem, JITTypeLib& types, JITConstCache& consts, const StubFunction* func, bool hasDirectParamAccess, bool emitExceptions, bool emitLines);
            ~JITFunctionWriterC();

            // emit function opcodes, returns generated code or empty strings on errors
            bool emitOpcodes(StringBuf& outCode);

        private:
            mem::LinearAllocator& m_mem;
            JITTypeLib& m_types;
            JITConstCache& m_consts;
            const StubFunction* m_func;

            StringBuilder m_prolog;
            StringBuilder m_code;
            bool m_hasErrors;
            bool m_hasDirectParamAccess;
            bool m_emitExceptions;
            bool m_emitLines;

            uint32_t m_lastLine;

            void reportError(const Stub* op, StringView<char> txt);

            //--

            StringView<char> copyString(const StringView<char> str);
            JITCodeChunkC makeChunk(const StubOpcode* op, const JITType* type, StringView<char> str);

            JITCodeChunkC processOpcode(StringView<char> contextStr, const StubOpcode* op, JITOpcodeStream& stream);
            bool processStatement(StringView<char> contextStr, const StubOpcode* op, JITOpcodeStream& stream);
            bool finishStatement(const StubOpcode* op);

            //--

            HashMap<const StubOpcode*, StringView<char>> m_labelMap;
            StringView<char> makeLabel(const StubOpcode* label);

            //--

            struct NewTemps
            {
                StringView<char> varName;
                const JITType* jitType = nullptr;
            };

            HashMap<int, JITCodeChunkC> m_localsMap;
            Array<NewTemps> m_newTemps;
            uint32_t m_tempVarCounter = 0;
            uint32_t m_exitLabelCounter = 0;
            bool m_exitLabelNeeded = false;

            JITCodeChunkC makePointer(const JITCodeChunkC& chunk);
            JITCodeChunkC makeValue(const JITCodeChunkC& chunk);
            JITCodeChunkC makeBinaryOp(const StubOpcode* context, const JITType* retType, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b);
            JITCodeChunkC makeBinaryOp(const StubOpcode* context, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b);
            JITCodeChunkC makeBinaryOp(const StubOpcode* context, StringID engineType, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b);
            JITCodeChunkC makeUnaryOp(const StubOpcode* context, const JITType* retType, const char* txt, const JITCodeChunkC& a);
            JITCodeChunkC makeUnaryOp(const StubOpcode* context, const char* txt, const JITCodeChunkC& a);
            JITCodeChunkC makeUnaryOp(const StubOpcode* context, StringID engineType, const char* txt, const JITCodeChunkC& a);

            JITCodeChunkC makeLocalVar(const StubOpcode* context, int localIndex, const Stub* type, StringID knownName = StringID());
            JITCodeChunkC makeTempVar(const StubOpcode* context, const JITType* requiredType);

            StringView<char> makeStatementExitLabel();

            void makeCtor(const JITType* type, bool isMemoryZeroed, StringView<char> str, bool fromPointer);
            void makeDtor(const JITType* type, StringView<char> str, bool fromPointer);
            void makeCopy(const JITType* type, StringView<char> to, bool targetPointer, StringView<char> from, bool fromPointer);

            void makeCopy(const JITCodeChunkC& to, const JITCodeChunkC& from);
            void makeCtor(const JITCodeChunkC& var, bool isMemoryZeroed);
            void makeDtor(const JITCodeChunkC& var);
        };

        //--

    } // script
} // base