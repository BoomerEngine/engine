/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\optimizer #]
***/

#pragma once

#include "renderingShaderProgram.h"
#include "renderingShaderStaticExecution.h"

#include "base/containers/include/queue.h"

namespace rendering
{
    namespace compiler
    {

        //---

        struct FoldedFunctionKey
        {
            const Function* func = nullptr;
            const ProgramInstance* pi = nullptr;
            uint64_t localArgsKey = 0;

            INLINE FoldedFunctionKey() {};

            INLINE static uint32_t CalcHash(const FoldedFunctionKey& key);

            INLINE bool operator==(const FoldedFunctionKey& other) const { return (func == other.func) && (pi == other.pi) && (localArgsKey == other.localArgsKey); }
            INLINE bool operator!=(const FoldedFunctionKey& other) const { return !operator==(other); }
        };

        ///---

        /// generates folded versions of shader functions
        class FunctionFolder : public base::NoCopy
        {
        public:
            FunctionFolder(base::mem::LinearAllocator& mem, CodeLibrary& code);
            ~FunctionFolder();

            /// Fold function code in context of given program instance
            /// NOTE: this is function -> function transformation
            /// NOTE: folding removes the dependencies on program instancing parameters as all constants are resolved and integrated into compiled function code
            /// NOTE: FunctionFolder is a cache, we will return reused result if possible
            Function* foldFunction(const Function* original,  // code
                const ProgramInstance* thisVars,  // "this vars" - parameterization of program instance 
                const ProgramConstants& localArgs,  // local arguments for calls
                base::parser::IErrorReporter& err);

        private:
            base::mem::LinearAllocator& m_mem;
            CodeLibrary& m_code;

            base::HashMap<FoldedFunctionKey, Function*> m_foldedFunctionsMap;
            base::HashMap<base::StringID, uint32_t> m_functionNameCounter;

            CodeNode* foldCode(Function* func, const CodeNode* node, const ProgramInstance* thisArgs, const ProgramConstants& localArgs, base::parser::IErrorReporter& err);
        };

        //---

    } // compiler
} // rendering
