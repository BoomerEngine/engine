/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once


#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/hashSet.h"
#include "base/parser/include/textToken.h"
#include "base/parser/include/textErrorReporter.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderDataValue.h"
#include "renderingShaderDataType.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderProgram.h"

namespace rendering
{
    namespace compiler
    {
        //--

        struct ExportedShaders
        {
            const Program* exports[(int)ShaderType::MAX];

            INLINE ExportedShaders()
            {
                memset(exports, 0, sizeof(exports));
            }
        };
       
        struct ResolvedDescriptorEntry
        {
            const ResourceTable* table = nullptr;
            const ResourceTableEntry* entry = nullptr;
            const CompositeType::Member* member = nullptr;
        };

        // code library
        class RENDERING_COMPILER_API CodeLibrary : public base::NoCopy
        {
        public:
            CodeLibrary(base::mem::LinearAllocator& allocator, TypeLibrary& typeLibrary);
            ~CodeLibrary();

            //--

            /// process shader content that was already tokenized, parses tokens and creates internal structures: shaders, types, etc
            /// NOTE: can be called few times with parts of content
            bool parseContent(base::parser::TokenList& tokens, base::parser::IErrorReporter& err);

            //---

            // find the program by name
            const Program* findProgram(const base::StringID programName) const;

            // find the global function
            const Function* findGlobalFunction(const base::StringID name) const;

            // find the global constant
            const DataParameter* findGlobalConstant(const base::StringID name) const;

            //---

            // create program declaration or a definition
            const Program* createProgram(base::StringID programName, const base::parser::Location& loc, AttributeList&& attributes);

            // create a unique program instance - a program parametrized with parameters
            const ProgramInstance* createProgramInstance(const base::parser::Location& loc, const Program* program, const ProgramConstants& params, base::parser::IErrorReporter& err);

            //---

            // get associated type library (global)
            INLINE TypeLibrary& typeLibrary() { return *m_typeLibrary; }

            // get associated type library (global)
            INLINE const TypeLibrary& typeLibrary() const { return *m_typeLibrary; }

            // get memory allocator for all parsing operations
            INLINE base::mem::LinearAllocator& allocator() const { return m_allocator; }

            // get all compiled programs
            typedef base::Array<const Program*> TProgramList;
            INLINE const TProgramList& programs() const { return m_programList; }

            // exported programs
            INLINE const ExportedShaders& exports() const { return m_exports; }

            //     

            // TODO: find better place

            void findDescriptorEntry(const ResourceTable* table, base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const;
            void findUniformEntry(const ResourceTable* table, base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const;
            
            void findGlobalDescriptorEntry(base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const;
            void findGlobalUniformEntry(base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const;

            bool resolveTypeInner(const parser::TypeReference* typeRef, DataType& outType, base::parser::IErrorReporter& err, bool reportErrors=true) const;
            bool resolveType(const parser::TypeReference* typeRef, DataType& outType, base::parser::IErrorReporter& err, bool reportErrors=true) const;

        private:
            // memory allocator for the parsing operations
            base::mem::LinearAllocator& m_allocator;

            // type library where are composite and enum types are registered
            TypeLibrary* m_typeLibrary;

            // all top-level global functions
            typedef base::HashMap<base::StringID, Function*> TGlobalFunctionMap;
            TGlobalFunctionMap m_globalFunctions;

            // all top-level global constants
            base::Mutex m_globalConstantsLock;
            typedef base::HashMap<base::StringID, DataParameter*> TGlobalConstantsMap;
            TGlobalConstantsMap m_globalConstants;

            // all top-level programs by name
            typedef base::HashMap<base::StringID, Program*> TProgram;
            TProgram m_programs;
            TProgramList m_programList;

            // all top-level exports
            ExportedShaders m_exports;

            // program instances
            typedef base::HashMap<uint64_t, ProgramInstance*> TProgramInstanceMap;
            TProgramInstanceMap m_programInstanceMap;
            base::Mutex m_programInstanceLock;

            //--

            // load shader file, if the file exists it's returned if the file does not exist it's loaded
            // used during library enumeration and in includes (as the file does not have to be a .frag)
            bool loadFile(base::parser::TokenList& tokens, base::parser::IErrorReporter& err);

            // create structure elements, programs, functions, constants
            bool createElements(const parser::Element* root, base::parser::IErrorReporter& err);
            bool createProgramElementsPass1(const parser::Element* elem, Program* program, base::parser::IErrorReporter& err);
            bool createProgramElementsPass2(const parser::Element* elem, Program* program, base::parser::IErrorReporter& err);
            bool createDescriptorElements(const parser::Element* elem, ResourceTable* program, base::parser::IErrorReporter& err);
            bool createStructureElements(const parser::Element* elem, CompositeType* program, base::parser::IErrorReporter& err);

            // process the code
            bool parseCode(base::parser::IErrorReporter& err);
        };

    } // compiler
} // rendering
