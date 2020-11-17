/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/app/include/localService.h"

namespace base
{
    namespace script
    {
        //----

        namespace jit
        {

            typedef void (*TNativeFunctionPtr)();

            struct EngineToJIT
            {
                void* self;

                void (*_fnLog)(void* self, const char* txt) = nullptr;
                void (*_fnThrowException)(void* self, const rtti::IFunctionStackFrame* frame, const char* file, int line, const char* txt) = nullptr;

                void (*_fnTypeCtor)(void* self, int typeId, void* data) = nullptr;
                void (*_fnTypeDtor)(void* self, int typeId, void* data) = nullptr;
                void (*_fnTypeCopy)(void* self, int typeId, void* dest, void* src) = nullptr;
                int (*_fnTypeCompare)(void* self, int typeId, void* a, void* b) = nullptr;

                void (*_fnCall)(void* self, void* context, int funcId, int mode, const rtti::IFunctionStackFrame* parentFrame, rtti::FunctionCallingParams* params) = nullptr;

                void (*_fnNew)(void* self, const rtti::IFunctionStackFrame* parentFrame, const ClassType* classPtr, ObjectPtr* strongPtr);
                bool (*_fnWeakToBool)(void* self, ObjectWeakPtr* weakPtr);
                void (*_fnWeakToStrong)(void* self, ObjectWeakPtr* weakPtr, ObjectPtr* strongPtr);
                void (*_fnStrongToWeak)(void* self, ObjectPtr* strongPtr, ObjectWeakPtr* weakPtr);
                void (*_fnStrongFromPtr)(void* self, void* ptr, ObjectPtr* strongPtr);
                StringID (*_fnEnumToName)(void* self, int typeId, int64_t enumValue);
                int64_t (*_fnNameToEnum)(void* self, const rtti::IFunctionStackFrame* parentFrame, int typeId, StringID enumName);
                void (*_fnDynamicStrongCast)(void* self, const ClassType* classPtr, ObjectPtr* inStrongPtr,  ObjectPtr* outStrongPtr);
                void (*_fnDynamicWeakCast)(void* self, const ClassType*classPtr, ObjectWeakPtr* inWeakPtr, ObjectWeakPtr* outWeakPtr);
                void (*_fnMetaCast)(void* self, const ClassType* classPtr, ClassType* inClassPtr, ClassType* outClassPtr);
                StringID (*_fnClassToName)(void* self, const ClassType* classPtr);
                void (*_fnClassToString)(void* self, const ClassType* classPtr, StringBuf* outString);
            };

            struct JITInit
            {
                void* self;
                void (*_fnReportImportCounts)(void* self, int maxTypeId, int maxFuncId) = nullptr;
                void (*_fnReportImportType)(void* self, int typeId, const char* name) = nullptr;
                void (*_fnReportImportFunction)(void* self, int funcId, const char* className, const char* funcName, TNativeFunctionPtr* nativeFunctionPointer) = nullptr;
                void (*_fnReportExportFunction)(void* self, const char* className, const char* funcName,  uint64_t codeHash, rtti::TFunctionJittedWrapperPtr funcPtr) = nullptr;
                void (*_fnInitStringConst)(void* self, void* str, const char* data) = nullptr;
                void (*_fnInitNameConst)(void* self, void* str, const char* data) = nullptr;
                void (*_fnInitTypeConst)(void* self, void* str, const char* data) = nullptr;;
            };

            typedef void (*TInitFunc)(EngineToJIT* engine, JITInit* init);

        } // jit

        //----

        // JITed script module, usually a DLL or other similar crap
        class BASE_SCRIPT_API JITProject : public IReferencable
        {
        public:
            JITProject(void* handle, StringView path);
            ~JITProject();

            //--

            // get path to file on disk
            INLINE StringView absolutePath() const { return m_path; }

            // get the system module handle
            INLINE void* handle() const { return m_handle; }

            // is this module bound ?
            INLINE bool isBound() const { return m_bound; }

            //--

            // bind to runtime environment
            bool bind();

            //--

            // load module from file, may file
            static JITProjectPtr Load(StringView path);

            // assemble a JIT project from compiled scripted module
            static JITProjectPtr Compile(const CompiledProjectPtr& project);

        private:
            StringBuf m_path;
            void* m_handle; // system handle
            bool m_bound;
            jit::EngineToJIT m_interface;

            // find internal function in the module
            void* findFunction(const char* name) const;

            //--

            // resolved data used in the JITed module, passed during initialization
            Array<Type> m_typeTable;
            Array<const rtti::Function*> m_functionTable;

            //--

            static void InterfaceLog(void* self, const char* txt);
            static void InterfaceThrowException(void* self, const rtti::IFunctionStackFrame* frame, const char* file, int line, const char* txt);
            static void InterfaceTypeCtor(void* self, int typeId, void* data);
            static void InterfaceTypeDtor(void* self, int typeId, void* data);
            static void InterfaceTypeCopy(void* self, int typeId, void* dest, void* src);
            static int InterfaceTypeCompare(void* self, int typeId, void* a, void* b);
            static void InterfaceCall(void* self, void* context, int funcId, int mode, const rtti::IFunctionStackFrame* parentFrame, rtti::FunctionCallingParams* params);
            static void InterfaceNew(void* self, const rtti::IFunctionStackFrame* parentFrame, const ClassType*classPtr, ObjectPtr* strongPtr);
            static bool InterfaceWeakToBool(void* self, ObjectWeakPtr* weakPtr);
            static void InterfaceWeakToStrong(void* self, ObjectWeakPtr* weakPtr, ObjectPtr* strongPtr);
            static void InterfaceStrongToWeak(void* self, ObjectPtr* strongPtr, ObjectWeakPtr* weakPtr);
            static void InterfaceStrongFromPtr(void* self, void* ptr, ObjectPtr* strongPtr);
            static StringID InterfaceEnumToName(void* self, int typeId, int64_t enumValue);
            static int64_t InterfaceNameToEnum(void* self, const rtti::IFunctionStackFrame* parentFrame, int typeId, StringID enumName);
            static void InterfaceDynamicStrongCast(void* self, const ClassType* classPtr, ObjectPtr* inStrongPtr,  ObjectPtr* outStrongPtr);
            static void InterfaceDynamicWeakCast(void* self, const ClassType* classPtr, ObjectWeakPtr* inWeakPtr, ObjectWeakPtr* outWeakPtr);
            static void InterfaceMetaCast(void* self, const ClassType* classPtr, ClassType* inClassPtr, ClassType* outClassPtr);
            static StringID InterfaceClassToName(void* self, const ClassType* classPtr);
            static void InterfaceClassToString(void* self, const ClassType* classPtr, StringBuf* outString);
        };

        //----

        //--

        /// interface used to gain insight into structure of native types
        /// NOTE: this is decoupled from direct RTTI access to allow cross-platform JITing (ie. data layout on IOS may be different than on PC, etc)
        class BASE_SCRIPT_API IJITNativeTypeInsight : public NoCopy
        {
        public:
            virtual ~IJITNativeTypeInsight();

            struct MemberInfo
            {
                StringID name;
                StringID typeName;
                uint32_t runtimeOffset = 0;
            };

            struct OptionInfo
            {
                StringID name;
                int64_t value = 0;
            };

            struct TypeInfo
            {
                rtti::MetaType metaType = rtti::MetaType::Void;
                uint32_t runtimeSize = 0;
                uint32_t runtimeAlign = 0;
                StringID innerTypeName; // arrays
                uint32_t staticArraySize = 0; // arrays
                StringID baseClassName;
                bool requiresConstructor = true;
                bool requiresDestructor = true;
                bool simpleCopyCompare = false;
                bool zeroInitializationConstructor = false;

                Array<MemberInfo> localMembers;
                Array<OptionInfo> options;

                INLINE operator bool() const
                {
                    return metaType != rtti::MetaType::Void;
                }
            };

            virtual TypeInfo typeInfo(StringID typeName) const = 0;

            //--

            // get the runtime (current) type insight - allows to JIT agains the current runtime types
            static const IJITNativeTypeInsight& GetCurrentTypes();
        };

        //----

        // JIT helper that transforms compiled script project into a compiled code
        class BASE_SCRIPT_API IJITCompiler : public IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IJITCompiler);

        public:
            virtual ~IJITCompiler();

            /// settings for the JIT compiler
            struct Settings
            {
                bool emitSymbols = false; // emit debug symbols
                bool emitOriginalLines = false; // emit mapping to original lines from source code
                bool emitExceptions = false; // emit the "null pointer", "divison by zero" and other exceptions
            };

            /// compile a JITed (or AOT, depends on how you look) version of the script code
            virtual bool compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& data, StringView outputModulePath, const Settings& settings) = 0;
        };

        //----

    } // script
} // base