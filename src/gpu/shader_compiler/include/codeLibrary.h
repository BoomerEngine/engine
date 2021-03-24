/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\nodes #]
***/

#pragma once


#include "core/memory/include/linearAllocator.h"
#include "core/containers/include/hashMap.h"
#include "core/containers/include/hashSet.h"
#include "core/parser/include/textToken.h"
#include "core/parser/include/textErrorReporter.h"
#include "fileParser.h"
#include "dataValue.h"
#include "dataType.h"
#include "typeLibrary.h"
#include "program.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//--

struct Exporteds
{
    const Program* exports[(int)ShaderStage::MAX];

    INLINE Exporteds()
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
class GPU_SHADER_COMPILER_API CodeLibrary : public NoCopy
{
public:
    CodeLibrary(LinearAllocator& allocator, TypeLibrary& typeLibrary);
    ~CodeLibrary();

    //--

    /// process shader content that was already tokenized, parses tokens and creates internal structures: s, types, etc
    /// NOTE: can be called few times with parts of content
    bool parseContent(TokenList& tokens, ITextErrorReporter& err);

    //---

    // find the program by name
    const Program* findProgram(const StringID programName) const;

    // find the global function
    const Function* findGlobalFunction(const StringID name) const;

    // find the global constant
    const DataParameter* findGlobalConstant(const StringID name) const;

    //---

    // create program declaration or a definition
    const Program* createProgram(StringID programName, const TextTokenLocation& loc, AttributeList&& attributes);

	// create instance of program (program + parametrization)
	const ProgramInstance* createProgramInstance(const TextTokenLocation& loc, const Program* program, const ProgramConstants& sourceParams, ITextErrorReporter& err);

    //---

    // get associated type library (global)
    INLINE TypeLibrary& typeLibrary() { return *m_typeLibrary; }

    // get associated type library (global)
    INLINE const TypeLibrary& typeLibrary() const { return *m_typeLibrary; }

    // get memory allocator for all parsing operations
    INLINE LinearAllocator& allocator() const { return m_allocator; }

    // get all compiled programs
    typedef Array<const Program*> TProgramList;
    INLINE const TProgramList& programs() const { return m_programList; }

    // exported programs
    INLINE const Exporteds& exports() const { return m_exports; }

    //     

    // TODO: find better place

    void findDescriptorEntry(const ResourceTable* table, StringID name, Array<ResolvedDescriptorEntry>& outEntries) const;
    void findUniformEntry(const ResourceTable* table, StringID name, Array<ResolvedDescriptorEntry>& outEntries) const;
            
    void findGlobalDescriptorEntry(StringID name, Array<ResolvedDescriptorEntry>& outEntries) const;
    void findGlobalUniformEntry(StringID name, Array<ResolvedDescriptorEntry>& outEntries) const;

    bool resolveTypeInner(const TypeReference* typeRef, DataType& outType, ITextErrorReporter& err, bool reportErrors=true) const;
    bool resolveType(const TypeReference* typeRef, DataType& outType, ITextErrorReporter& err, bool reportErrors=true) const;

private:
    // memory allocator for the parsing operations
    LinearAllocator& m_allocator;

    // type library where are composite and enum types are registered
    TypeLibrary* m_typeLibrary;

    // all top-level global functions
    typedef HashMap<StringID, Function*> TGlobalFunctionMap;
    TGlobalFunctionMap m_globalFunctions;

    // all top-level global constants
    Mutex m_globalConstantsLock;
    typedef HashMap<StringID, DataParameter*> TGlobalConstantsMap;
    TGlobalConstantsMap m_globalConstants;

    // all top-level programs by name
    typedef HashMap<StringID, Program*> TProgram;
    TProgram m_programs;
    TProgramList m_programList;

	// program instances
	typedef HashMap<uint64_t, ProgramInstance*> TProgramInstanceMap;
	TProgramInstanceMap m_programInstanceMap;
	Mutex m_programInstanceLock;

    // all top-level exports
    Exporteds m_exports;

    //--

    // load shader file, if the file exists it's returned if the file does not exist it's loaded
    // used during library enumeration and in includes (as the file does not have to be a .frag)
    bool loadFile(TokenList& tokens, ITextErrorReporter& err);

    // create structure elements, programs, functions, constants
    bool createElements(const Element* root, ITextErrorReporter& err);
    bool createProgramElementsPass1(const Element* elem, Program* program, ITextErrorReporter& err);
    bool createProgramElementsPass2(const Element* elem, Program* program, ITextErrorReporter& err);
    bool createDescriptorElements(const Element* elem, ResourceTable* program, ITextErrorReporter& err);
    bool createStructureElements(const Element* elem, CompositeType* program, ITextErrorReporter& err);

    // process the code
    bool parseCode(ITextErrorReporter& err);
};

END_BOOMER_NAMESPACE_EX(gpu::compiler)
