/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "base/script/include/scriptOpcodes.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/containers/include/hashSet.h"
#include "base/memory/include/linearAllocator.h"

#include "scriptPortableStubs.h"
#include "scriptFunctionRuntimeCode.h"

namespace base
{
    namespace script
    {

        //----

        class TypeRegistry;
        class ScriptedClass;
        class ScriptedStruct;

        /// temporary helper class that can be used to load scripts into the environment
        class BASE_SCRIPT_API Loader : public IFunctionCodeStubResolver
        {
        public:
            Loader(TypeRegistry& registry);
            ~Loader();

            // link scripted module with into one big symbol table
            void linkModule(const PortableData& data);

            // validate state and load new scripts
            // NOTE: if the validation fails no state is changed
            bool validate();

            // apply loaded data
            void createExports();

        private:
            struct Symbol : public NoCopy
            {
                RTTI_DECLARE_POOL(POOL_SCRIPT_STREAM)

            public:
                StubType m_stubType;
                StringID m_fullName; // name in the scripts, note can be engine name

                const Stub* m_exportStub = nullptr;
                Array<const Stub*> m_importStubs;

                Symbol* m_classOwner = nullptr; // class functions only, NULL for global functions

                INLINE const Stub* anyStub() const
                {
                    return m_exportStub ? m_exportStub : m_importStubs.front();
                }

                INLINE bool resolved() const
                {
                    return (m_resolved.m_function != nullptr);
                }

                union
                {
                    const rtti::Function *m_function = nullptr;
                    const rtti::IClassType* m_class;
                    const rtti::EnumType *m_enum;
                    const rtti::Property *m_property;
                } m_resolved;
            };

            Symbol* createSymbol(StringView name, const Stub* stub, bool import);

            HashMap<StringID, Symbol*> m_symbolNamedMap;
            HashMap<const Stub*, Symbol*> m_symbolStubMap;
            Array<Symbol*> m_allSymbols;
            uint32_t m_numImports = 0;
            uint32_t m_numExports = 0;

            struct TypeDecl
            {
                const StubTypeDecl* m_stub;
                Type m_resolvedType = nullptr;
            };

            Array<TypeDecl> m_allTypeDecls;
            HashMap<const StubTypeDecl*, int> m_typeMap;

            Array<Symbol*> m_allClasses;
            Array<Symbol*> m_allEnums;
            Array<Symbol*> m_allProps;
            Array<Symbol*> m_allFunc;

            TypeRegistry& m_typeRegistry;

            //--

            bool findParentSymbols();
            bool matchImportExports();
            bool resolveEngineImports();
            bool checkAliasingWithEngine(Symbol* symbol);
            bool ensureSymbolsResolvable();

            Type createType(const StubTypeDecl* typeDecl);
            Type createType(const StubTypeRef* typeRef);

            bool updateStructSizes();
            bool updateClassSizes();

            //--

            static bool MatchTypeRef(const StubTypeRef* a, const StubTypeRef* b);
            static bool MatchTypeDecl(const StubTypeDecl* a, const StubTypeDecl* b);
            static bool MatchFunctionArg(const StubFunctionArg* a, const StubFunctionArg* b);
            static bool MatchFunctionDecl(const StubFunction* a, const StubFunction* b);
            static bool MatchEnumDecl(const StubEnum* a, const StubEnum* b);
            static bool MatchPropertyDecl(const StubProperty* a, const StubProperty* b);
            static bool MatchClassDecl(const StubClass* a, const StubClass* b);
            static bool MatchClassRef(const StubClass* a, const StubClass* b);
            static bool MatchClassStub(const Stub* a, const Stub* b);
            static bool MatchStub(const Stub* a, const Stub* b);

            static bool IsStructClass(ClassType classType);
            static bool MatchPropertyType(Type refType, const Stub* resolvedStub);
            static bool MatchPropertyType(Type refType, const StubTypeRef* decl);
            static bool MatchPropertyType(Type refType, const StubTypeDecl* decl);
            static bool MatchFunctionSignature(const StubFunction* stubFunc, const rtti::Function* engineFunc);

            static ClassType FindNativeClassBase(const StubClass* scriptedClass);

            //--

            virtual Type resolveType(const StubTypeDecl* stub) override final;
            virtual ClassType resolveClass(const StubClass* stub) override final;
            virtual const rtti::EnumType* resolveEnum(const StubEnum* stub) override final;
            virtual const rtti::Property* resolveProperty(const StubProperty* prop) override final;
            virtual const rtti::Function* resolveFunction(const StubFunction* func) override final;
        };


        //----

    } // script
} // base

