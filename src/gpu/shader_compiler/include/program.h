/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#pragma once

#include "typeLibrary.h"
#include "codeNode.h"
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

class PermutationTable;
class ResourceTable;
struct ResourceTableEntry;
struct ResolvedDescriptorEntry;

//---

/// a shader program definition
class GPU_SHADER_COMPILER_API Program : public NoCopy
{
public:
    Program(const CodeLibrary& library, StringID name, const TextTokenLocation& loc, AttributeList&& attributes);
    ~Program();

    // get shader library this function is coming from
    INLINE const CodeLibrary& library() const { return *m_library; }

    // get location of function in the file
    INLINE const TextTokenLocation& location() const { return m_loc; }

    // get general attributes (from parser)
    INLINE const AttributeList& attributes() const { return m_attributes; }

    // get general attributes (from parser)
    INLINE AttributeList& attributes() { return m_attributes; }

    // get program parameters
    typedef Array<DataParameter*> TParameters;
    INLINE const TParameters& parameters() const { return m_parameters; }

    // get program functions
    typedef Array<Function*> TFunctions;
    INLINE const TFunctions& functions() const { return m_functions; }

    // get parent programs
    typedef Array<const Program*> TParentPrograms;
    INLINE const TParentPrograms& parentPrograms() const { return m_parentPrograms; }

	// get render states used by the program
	typedef Array<const StaticRenderStates*> TStaticRenderStates;
	INLINE const TStaticRenderStates& staticRenderStates() const { return m_staticRenderStates; }

    // get list of referenced descriptors 
    // NOTE: given descriptor may be NOT used in specific permutation, this is upper bound
    typedef Array<const ResourceTable*> TDescriptors;
    INLINE const TDescriptors& descriptors() const { return m_descriptors; }

    // get the name of the function
    INLINE StringID name() const { return m_name; }

    //---

    // add parent program (we will suck functions and constants from it)
    void addParentProgram(const Program* parentProgram);

    // add program parameter
    void addParameter(DataParameter* param);

    // add program function definition
    void addFunction(Function* func);

	// add render states to use with this program
	void addRenderStates(const StaticRenderStates* states);

    //---

    // check if this program depends on given other program
    bool isBasedOnProgram(const Program* program) const;

    //---

    // find attribute in the program by name
    // NOTE: may recurse to the parent programs
    const DataParameter* findParameter(const StringID name, bool recurseToParent = true) const;

    // find function in the program by name
    // NOTE: may recurse to the parent programs
    const Function* findFunction(const StringID function, bool recurseToParent = true) const;

    //---

    /*// refresh the CRC of the program, returns false the CRC cannot be computed
    bool refreshCRC();*/

    //---

    // build data param from a descriptor constant buffer field or resource
    const DataParameter* createDescriptorElementReference(const ResolvedDescriptorEntry& entry) const;

    // resolve a built-in parameter
    const DataParameter* createBuildinParameterReference(StringID name) const;

    //---

    // print to debug text
    void print(IFormatStream& f) const;

private:
    StringID m_name; // a user-given program name
    mutable TParameters m_parameters; // all parameters of the program
    mutable TDescriptors m_descriptors; // all referenced descriptors
    TFunctions m_functions; // all program functions
    TParentPrograms m_parentPrograms; // parent programs we extend/implement
	TStaticRenderStates m_staticRenderStates; // static graphics pipeline configuration
    AttributeList m_attributes; // as defined in the shader code

    TextTokenLocation m_loc; // location in source file

    const CodeLibrary* m_library; // source library this function was compiled as part of

    Array<TextTokenLocation> m_refLocs;

	mutable HashMap<StringBuf, DataParameter*> m_descriptorConstantBufferEntriesMap;
	mutable HashMap<StringBuf, DataParameter*> m_descriptorResourceMap;

    friend class ParsingContext;
    friend class FunctionContext;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
