/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

namespace base
{
    namespace script
    {

        //----

        class PortableData;
        class TypeRegistry;
        class ScriptedClass;
        class ScriptedStruct;

        struct Stub;
        struct StubModule;
        struct StubFunction;
        struct StubTypeDecl;
        struct StubTypeName;
        struct StubProperty;
        struct StubEnum;
        struct StubClass;

        //----

        // top level scripting environment, contains binding
        class BASE_SCRIPT_API Environment : public base::NoCopy
        {
        public:
            Environment();
            ~Environment();

            // load the compiled scripts package, called very early during service startup
            // NOTE: engine most likely will not run without those compiled scripts
            bool load();

        private:
            CompiledProjectPtr m_compiled; // runtime handle to compiled data
            JITProjectPtr m_jit; // JIt of the project's code

            UniquePtr<TypeRegistry> m_types;
        };

        //----

    } // script
} // base