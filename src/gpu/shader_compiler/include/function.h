/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#pragma once

#include "codeNode.h"
#include "typeLibrary.h"
#include "gpu/device/include/shaderStubs.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

class FunctionFolder;

//---

/// type of shader symbol usage
/// we support following symbols: inputs, outputs, parameters and functions
enum class DataParameterScope : uint8_t
{
    Unknown, // invalid value
    StaticConstant, // constant that is static for the whole compilation
    GlobalConst, // value is read only constant that CAN be overridden via the parametrization table
    GlobalParameter, // value is a read only constant outside the shader pipeline (uniform) that is NOT parameterizable
    GlobalBuiltin, // builtin variable
    VertexInput, // VA input to vertex shader
    StageInput, // value is consumed by stage
    StageOutput, // value is written out by stage
    GroupShared, // value is d withing the working group
    FunctionInput, // function argument, not writable
    ScopeLocal, // local variable in a scope, should not be seen outside the scope
    Export, // export constant
};
        
/// shader symbol information
/// we support following symbols: inputs, outputs, parameters
struct GPU_SHADER_COMPILER_API DataParameter
{
    StringID name; // a user-given name, not modified
    DataParameterScope scope = DataParameterScope::Unknown; // fine grain detail (is this an input/output or a constant value)
    AttributeList attributes;

    DataType dataType; // data type defined for parameter
	bool assignable = false;

    parser::Location loc; // location in source file

    Array<parser::Token*> initalizationTokens; // initialization tokens (can be a full expression)
    CodeNode* initializerCode = nullptr; // parsed initialization code for the value
	bool constant = false;

	const ResourceTable* resourceTable = nullptr;
	const ResourceTableEntry* resourceTableEntry = nullptr;
	const CompositeType::Member* resourceTableCompositeEntry = nullptr;
	shader::ShaderBuiltIn builtInVariable = shader::ShaderBuiltIn::Invalid;

    void print(IFormatStream& f) const;
};

//---

/// a function in the shader
class GPU_SHADER_COMPILER_API Function : public NoCopy
{
public:
    Function(const CodeLibrary& library, const Program* program, const parser::Location& loc, const StringID name, const DataType& retType, const Array<DataParameter*>& params, const Array<parser::Token*>& tokens, AttributeList&& attributes);
	Function(const Function& func, const ProgramConstants* localConstantArgs = nullptr);
    ~Function();

    // get shader library this function is coming from
    INLINE const CodeLibrary& library() const { return *m_library; }

    // get the program this function belongs to
    INLINE const Program* program() const { return m_program; }

    // get location of function in the file
    INLINE const parser::Location& location() const { return m_loc; }

    // get source tokens
    INLINE const Array<parser::Token*>& tokens() const { return m_codeTokens; }
            
    // attributes (from source code) + dynamic
    // NOTE: we mostly care about attributes on main() function in s
    INLINE const AttributeList& attributes() const { return m_attributes; }

    // get the return type of the function
    INLINE const DataType& returnType() const { return m_returnType; }

    // get function input parameters
    INLINE const Array<DataParameter*>& inputParameters() const { return m_inputParameters; }

	// get list of static (predefined) function parameters
	INLINE const Array<StringID>& staticParameters() const { return m_staticParameters; }

    // get the name of the function
    INLINE StringID name() const { return m_name; }

    // get function code
    INLINE const CodeNode& code() const { return *m_code; }

    //---

    // bind compiled code to function
    void bindCode(CodeNode* code);

    // rename function
    void rename(StringID name);

    //---

    // print to debug text
    void print(IFormatStream& f) const;

private:
    StringID m_name; // a user-given function name
    DataType m_returnType; // type of value returned by function

    CodeNode* m_code; // code the function generates
    Array<parser::Token*> m_codeTokens; // code tokens

    Array<DataParameter*> m_inputParameters; // input parameters of the function
	Array<StringID> m_staticParameters;

    AttributeList m_attributes; // user defined function attributes

    uint64_t m_foldedKey = 0; // unique identifier of function's code

    parser::Location m_loc; // location in source file

    const CodeLibrary* m_library; // source library this function was compiled as part of
    const Program* m_program; // program this function belongs to

    friend class ParsingContext;
    friend class FunctionContext;
    friend class FunctionFolder;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
