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

        struct JITType;

        /// JIT type type
        enum class JITMetaType : uint8_t
        {
            Engine, // opaque engine type, may be simple
            Enum,
            Class,
            ClassPtr,
            StrongPtr,
            WeakPtr,
            DynamicArray,
            StaticArray,
        };

        /// JIT type member
        struct JITClassMember
        {
            StringID name;
            uint32_t runtimeOffset = 0;
            bool extendedBuffer = false; // only scripted classes
            bool native = false; // defined in C++
            const JITType* jitType = nullptr;
        };

        /// JIT type
        struct JITType
        {
            const Stub* stub = nullptr;

            JITMetaType metaType;

            bool native = false; // this is a native type defined in C++
            bool emitPrototype = false; // this is a native type defined in C++
            bool emitExtraPrototype = false; // this is a native type defined in C++
            bool imported = false; // type was declared as imported in the script (ie. must exist before, NOTE: does not imply native)
            int assignedID = -1;
            StringID name;
            StringView<char> jitName;
            StringView<char> jitExtraName; // scripted classes "scripted" part
            uint32_t runtimeSize = 0;
            uint32_t runtimeAlign = 1;
            uint32_t runtimeExtraSize = 0;  // scripted classes "scripted" part
            uint32_t runtimeExtraAlign = 1;
            bool requiresConstructor = true;
            bool requiresDestructor = true;
            bool simpleCopyCompare = false;
            bool zeroInitializedConstructor = false;

            uint32_t staticArraySize = 0; // arrays
            const JITType* innerType = nullptr; // arrays

            const JITType* baseClass = nullptr; // classes

            mutable StringView<char> ctorFuncName;
            mutable StringView<char> dtorFuncName;

            Array<JITClassMember> fields; // struct and classes
            HashMap<StringID, int64_t> enumOptions; // enums
        };

        /// Argument of imported function
        struct JITImportedFunctionArg
        {
            StringID name;
            const StubFunctionArg* stub = nullptr;
            const JITType* jitType = nullptr;
            bool passAsReference = false;
        };

        /// JIT imported function
        struct JITImportedFunction
        {
            int assignedID = -1;
            const JITType* jitClass = nullptr;
            StringView<char> functionName;
            StringView<char> jitName; // name of the call forwarder
            const StubFunction* stub = nullptr;

            const JITType* returnType;
            Array<JITImportedFunctionArg> args;

            bool canReturnDirectly = false; // if type is simple enough we can return it directly instead of the void* resultPtr shit

            StringView<char> jitDirectCallName; // if we determined that we can call this function directly this is the name of the variable holding the pointer to it
        };

        /// JIT type library
        class BASE_SCRIPT_JIT_API JITTypeLib : public NoCopy
        {
        public:
            JITTypeLib(mem::LinearAllocator& mem, const IJITNativeTypeInsight& nativeTypes);
            ~JITTypeLib();

            /// do we have errors ?
            INLINE bool hasErrors() const { return m_hasErrors; }

            //--

            /// get JIT type for given type stub
            const JITType* resolveType(const Stub* typeStub);

            /// get JIT type for given type declaration
            const JITType* resolveType(const StubTypeDecl* typeStub);

            /// map a function for calling
            const JITImportedFunction* resolveFunction(const StubFunction* func);

            /// get engine type
            const JITType* resolveEngineType(StringID engineTypeName);

            /// resolve final layout of members in types
            bool resolveTypeSizesAndLayouts();

            //--

            /// report local body of a function (so we may call it directly)
            void reportLocalFunctionBody(const StubFunction* func, StringView<char> localJitBodyName, bool fastCall);

            //--

            /// print structures declarations
            void printTypePrototypes(IFormatStream& f) const;

            /// print the declarations of "call forwarders" for imported functions
            void printCallForwarderDeclarations(IFormatStream& f) const;

            /// print the "call forwarders" for imported functions, allows to use simpler syntax for calling function
            void printCallForwarders(IFormatStream& f) const;

            /// print type and function imports
            void printImports(IFormatStream& f) const;

        private:
            mem::LinearAllocator& m_mem;
            const IJITNativeTypeInsight& m_nativeTypes;

            Array<const JITType*> m_allTypes;
            Array<const JITImportedFunction*> m_allFunctions;

            Array<JITType*> m_scriptedStructs;
            Array<JITType*> m_scriptedClasses;

            HashMap<const Stub*, JITType*> m_stubMap;
            HashMap<const StubTypeDecl*, JITType*> m_stubTypeDeclMap;
            HashMap<StringID, JITType*> m_engineTypeMap;
            HashMap<const StubFunction*, JITImportedFunction*> m_importedFunctionMap;

            struct LocalFuncInfo
            {
                StringView<char> m_bodyName;
                bool m_isFastCall;
            };

            HashMap<const StubFunction*, LocalFuncInfo> m_localyImplementedFunctionsMap;

            bool m_hasErrors;

            //--

            const JITType* createFromNativeType(StringID engineTypeName);
            const JITType* createScriptedEnum(const StubEnum* stub);
            const JITType* createScriptedClass(const StubClass* stub);
            const JITType* createScriptedStruct(const StubClass* stub);

            bool updateStructSizes();
            bool updateClassSizes();

            bool updateMemberLayout(JITType* type);
            bool updateScriptedMemberLayout(JITType* type);

            void reportError(const Stub* owner, StringView<char> txt);

            void collectFields(const JITType* type, bool extendedBuffer, Array<const JITClassMember*>& outMembers) const;
        };

        //--

    } // script
} // base