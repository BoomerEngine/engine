 %{

// [# filter: compiler\parser #]

#include "build.h"
#include "rendering/compiler/src/renderingShaderFileParserHelper.h"

#ifdef PLATFORM_MSVC
	#pragma warning ( disable: 4702 ) //: unreachable code
#endif

using ParsingNode = rendering::compiler::parser::ParsingNode;
using Element = rendering::compiler::parser::Element;
using ElementType = rendering::compiler::parser::ElementType;
using TypeReference = rendering::compiler::parser::TypeReference;
using ElementFlags = rendering::compiler::parser::ElementFlags;
using ElementFlag = rendering::compiler::parser::ElementFlag;

static int cslf_lex(rendering::compiler::parser::ParsingNode* outNode, rendering::compiler::parser::ParsingFileContext& ctx, rendering::compiler::parser::ParserTokenStream& tokens)
{
    return tokens.readToken(*outNode);
}

static int cslf_error(rendering::compiler::parser::ParsingFileContext& ctx, rendering::compiler::parser::ParserTokenStream& tokens, const char* txt)
{
    ctx.reportError(tokens.location(), base::TempString("parser error: {} near '{}'", txt, tokens.text()));
    return 0;
}

%}

%require "3.0"
%defines
%define api.pure full
%define api.prefix {cslf_};
%define api.value.type {rendering::compiler::parser::ParsingNode}
%define parse.error verbose

%parse-param { rendering::compiler::parser::ParsingFileContext& context }
%parse-param { rendering::compiler::parser::ParserTokenStream& tokens }

%lex-param { rendering::compiler::parser::ParsingFileContext& context }
%lex-param { rendering::compiler::parser::ParserTokenStream& tokens }

%token TOKEN_EXPORT
%token TOKEN_SHADER
%token TOKEN_DESCRIPTOR
%token TOKEN_CONSTANTS
%token TOKEN_STATIC_SAMPLER
%token TOKEN_RENDER_STATES
%token TOKEN_STRUCT
%token TOKEN_THIS
%token TOKEN_VERTEX

%token TOKEN_SHARED
%token TOKEN_IN
%token TOKEN_OUT
%token TOKEN_INOUT
%token TOKEN_CONST
%token TOKEN_ATTRIBUTE

%token TOKEN_INT_NUMBER
%token TOKEN_FLOAT_NUMBER
%token TOKEN_BOOL_TRUE
%token TOKEN_BOOL_FALSE
%token TOKEN_STRING
%token TOKEN_NAME
%token TOKEN_IDENT

%token TOKEN_CASTABLE_TYPE
%token TOKEN_ARRAY_TYPE
%token TOKEN_VECTOR_TYPE
%token TOKEN_MATRIX_TYPE
%token TOKEN_SHADER_TYPE
%token TOKEN_STRUCT_TYPE

%token TOKEN_BREAK
%token TOKEN_CONTINUE
%token TOKEN_RETURN
%token TOKEN_DISCARD
%token TOKEN_DO
%token TOKEN_WHILE
%token TOKEN_IF
%token TOKEN_ELSE
%token TOKEN_FOR

%token TOKEN_INC_OP
%token TOKEN_DEC_OP
%token TOKEN_LEFT_OP
%token TOKEN_RIGHT_OP
%token TOKEN_LE_OP
%token TOKEN_GE_OP
%token TOKEN_EQ_OP
%token TOKEN_NE_OP
%token TOKEN_AND_OP
%token TOKEN_OR_OP
%token TOKEN_MUL_ASSIGN
%token TOKEN_DIV_ASSIGN
%token TOKEN_MOD_ASSIGN
%token TOKEN_ADD_ASSIGN
%token TOKEN_SUB_ASSIGN
%token TOKEN_LEFT_ASSIGN
%token TOKEN_RIGHT_ASSIGN
%token TOKEN_AND_ASSIGN
%token TOKEN_XOR_ASSIGN
%token TOKEN_OR_ASSIGN

%% /* The grammar follows.  */

file
    : global_declaration_list { context.result() = $1; }
    ;

global_declaration_list
    : global_declaration_list global_declaration  { $$ = $1.link($2); }
    | /* empty */ { $$ = ParsingNode(context.alloc<Element>(tokens.location(), ElementType::Global)); }
    ;

global_declaration
    : attributes global_declaration_internal {
        $$.element = $2.element;
        if ($$.element)
            $$.element->attributes = std::move($1.elements);
    }
    ;

global_declaration_internal
	: struct_definition
	| descriptor_declaration
	| program_declaration
    | global_function_declaration
	| global_const_declaration
	| static_sampler_declaration
	| render_states_declaration
	;

struct_definition
    : TOKEN_STRUCT TOKEN_IDENT '{' struct_element_list '}' optional_semicolon
    {
        $$ = ParsingNode(context.alloc<Element>($1.location, ElementType::Struct, $2.stringData));
        $$.element->children = std::move($4.elements);
    }
    ;

struct_element_list
    : struct_element_list struct_element
    {
        $$.elements = std::move($1.elements);
        $$.elements.pushBack($2.element);
    }
    | /* empty */
    ;

struct_element
    : attributes type_declaration TOKEN_IDENT struct_element_initialization ';'
    {
        $$ = ParsingNode(context.alloc<Element>($3.location, ElementType::StructElement, $3.stringData));
        $$.element->attributes = std::move($1.elements);
        $$.element->typeRef = $2.typeRef;
        $$.element->tokens = $4.tokens;
    }
    ;

struct_element_initialization
    : '=' { tokens.extractInnerTokenStream(';', $$.tokens); yyclearin; }
    | { $$.tokens.clear(); } /* empty */
    ;

//---

attributes
    : attribute_group attributes { $$.elements.pushBack($2.elements.typedData(), $2.elements.size()); } 
    | /*empty*/ { $$.elements.clear(); }
    ;

attribute_group
    : TOKEN_ATTRIBUTE '(' attribute_list ')' { $$.elements = std::move($3.elements); };
    ;

attribute_list
    : attribute_list ',' attribute  { $$.elements.pushBack($3.element); };
    | attribute { $$.elements.reset(); $$.elements.pushBack($1.element); }
    | /* empty */ { $$.elements.reset(); }
    ;

attribute
    : attribute_key attribute_optional_value {
        $$ = ParsingNode(context.alloc<Element>($1.location, ElementType::Attribute, $1.stringData));
        $$.element->stringData = $2.stringData;
        $$.element->intData = $2.intData;
    }
    ;

attribute_optional_value
    : '=' attribute_value { $$.stringData = $2.stringData; $$.intData = $2.intData; }
    | /* empty */ { $$.intData = 0; $$.stringData = ""; }
    ;

attribute_key
	: TOKEN_IDENT
	| TOKEN_RENDER_STATES
	| TOKEN_STATIC_SAMPLER
	| TOKEN_DESCRIPTOR
	| TOKEN_CONST
	| TOKEN_SHADER
	| TOKEN_VERTEX
	| TOKEN_EXPORT
	| TOKEN_SHARED
	| TOKEN_DISCARD
	| TOKEN_ATTRIBUTE
	;

attribute_value
    : TOKEN_INT_NUMBER
    | TOKEN_VERTEX
    | TOKEN_SHADER
    | TOKEN_SHARED
    | TOKEN_IN
    | TOKEN_OUT
    | TOKEN_CONST
    | TOKEN_EXPORT
    | TOKEN_BOOL_TRUE
    | TOKEN_BOOL_FALSE
    | TOKEN_DISCARD
    | TOKEN_IDENT
    | TOKEN_STRING
    ;

//---

static_sampler_declaration
	: TOKEN_STATIC_SAMPLER TOKEN_IDENT '{' general_param_element_element_list optional_comma '}' optional_semicolon
	{
		$$ = ParsingNode(context.alloc<Element>($2.location, ElementType::StaticSampler, $2.stringData));
        $$.element->children = std::move($4.elements);
	} 
	;

render_states_declaration
	: TOKEN_RENDER_STATES TOKEN_IDENT '{' general_param_element_element_list optional_comma '}' optional_semicolon
	{
		$$ = ParsingNode(context.alloc<Element>($2.location, ElementType::RenderStates, $2.stringData));
        $$.element->children = std::move($4.elements);
	}
	;

general_param_element_element_list
    : general_param_element_element_list ',' general_param_element
    {
        $$.elements.pushBack($3.element);
    }
    | general_param_element
	{
		$$.elements.reset();
		$$.elements.pushBack($1.element);
	}
    ;

general_param_element
    : attributes TOKEN_IDENT '=' general_param_value
    {
        $$ = ParsingNode(context.alloc<Element>($3.location, ElementType::KeyValueParam, $2.stringData));
        $$.element->attributes = std::move($1.elements);
        $$.element->stringData = $4.stringData;
    }
    ;

general_param_value
	: TOKEN_IDENT
	| TOKEN_BOOL_TRUE
	| TOKEN_BOOL_FALSE
	| TOKEN_STRING
	| TOKEN_INT_NUMBER
	| TOKEN_FLOAT_NUMBER
	| TOKEN_NAME
	;

//---

descriptor_declaration
    : TOKEN_DESCRIPTOR TOKEN_IDENT '{' descriptor_element_list '}' optional_semicolon
    {
        $$ = ParsingNode(context.alloc<Element>($1.location, ElementType::Descriptor, $2.stringData));
        $$.element->children = std::move($4.elements);
    }
    ;
	
descriptor_element_list
    : descriptor_element_list descriptor_element
    {
        $$.elements.pushBack($2.element);
    }
    | /* empty */
	{
		$$.elements.reset();
	}
    ;

descriptor_element
    : attributes TOKEN_IDENT TOKEN_IDENT ';'
    {
        $$ = ParsingNode(context.alloc<Element>($3.location, ElementType::DescriptorResourceElement, $3.stringData));
        $$.element->attributes = std::move($1.elements);
        $$.element->stringData = $2.stringData;
    }

    | TOKEN_CONSTANTS '{' struct_element_list '}' optional_semicolon
    {
        $$ = ParsingNode(context.alloc<Element>($1.location, ElementType::DescriptorConstantTable));
        $$.element->children = std::move($3.elements);
    }

	| TOKEN_STATIC_SAMPLER TOKEN_IDENT ';'
    {
        $$ = ParsingNode(context.alloc<Element>($1.location, ElementType::DescriptorResourceElement, $2.stringData));
        $$.element->stringData = "Sampler";
    }

    ;

//---

program_declaration
    : program_type TOKEN_IDENT program_base_list program_stuff
    {
        $$ = ParsingNode(context.alloc<Element>($2.location, ElementType::Program, $2.stringData));
        $$.element->flags = $1.flags;

		for (auto* child : $3.elements)
			$$.element->children.pushBack(child);

		for (auto* child : $4.elements)
			$$.element->children.pushBack(child);
    }
    ;

program_stuff
    : '{' program_element_list '}' optional_semicolon { $$.elements = std::move($2.elements); }
    | /* empty */ ';' { $$.elements.clear(); }
    ;

program_type
    : TOKEN_EXPORT TOKEN_SHADER { $$.flags = ElementFlag::Export; }
    | TOKEN_SHADER { $$.flags = ElementFlag(); }
    ;

program_base_list
	: ':' program_base_list_elements { $$.elements = std::move($2.elements); }
	| /* empty */ { $$.elements.clear(); }
	;

program_base_list_elements
	: program_base_list_element {
		$$.elements.clear();
		$$.elements.pushBack($1.element);
	}
	| program_base_list_elements ',' program_base_list_element {
		$$.elements.pushBack($3.element);
	}
	;

program_base_list_element
	: TOKEN_IDENT {
		$$.element = context.alloc<Element>($1.location, ElementType::Using, $1.stringData);
	}
	;

program_element_list
    : program_element_list program_element
    {
        if ($2.element)
            $$.elements.pushBack($2.element);
    };
    | /*empty*/ { $$.elements.clear(); }
    ;

program_element
    : attributes program_element_flags variable_function_declaration {
        $$.element = $3.element;
        $$.element->flags = $2.flags;
        $$.element->attributes = std::move($1.elements);
    };
    ;	

program_element_flags
    : program_element_flag program_element_flags { $$.flags = $1.flags | $2.flags; };
    | /*emtpy*/ { $$.flags = ElementFlags(); }
    ;
	
program_element_flag
    : TOKEN_SHARED { $$.flags = ElementFlag::Shared; };
	| TOKEN_CONST { $$.flags = ElementFlag::Const; };
	| TOKEN_IN { $$.flags = ElementFlag::In; };
	| TOKEN_OUT { $$.flags = ElementFlag::Out; };
    | TOKEN_VERTEX { $$.flags = ElementFlag::Vertex; };
	;

//---

global_function_declaration
    : variable_function_header '(' function_argument_list ')' function_code '}'
    {
        $$.element = $1.element;
        $$.element->type = ElementType::Function;
        $$.element->children = std::move($3.elements);
        $$.element->tokens = $5.tokens;
    }
    ;

global_const_declaration
	: TOKEN_CONST variable_function_header variable_default ';'
    {
        $$.element = $2.element;
        $$.element->type = ElementType::Variable;
        $$.element->tokens = $3.tokens;
		$$.element->flags |= ElementFlag::Const;
    }
	;

variable_function_declaration
    : variable_function_header variable_default ';'
    {
        $$.element = $1.element;
        $$.element->type = ElementType::Variable;
        $$.element->tokens = $2.tokens;
    }
    | variable_function_header '(' function_argument_list ')' function_code '}'
    {
        $$.element = $1.element;
        $$.element->type = ElementType::Function;
        $$.element->children = std::move($3.elements);
        $$.element->tokens = $5.tokens;
    }
    ;

variable_function_header
    : type_declaration TOKEN_IDENT
    {
        $$ = ParsingNode(context.alloc<Element>($2.location, ElementType::Function, $2.stringData));
        $$.element->typeRef = $1.typeRef;
    }
    ;

variable_default
    : '=' { tokens.extractInnerTokenStream(';', $$.tokens); yyclearin; }
    | { $$.tokens.clear(); } /* empty */
    ;

//---

function_argument_list
    : function_argument_list ',' function_argument
    {
        $$.elements = std::move($1.elements);
        $$.elements.pushBack($3.element);
    }
    | function_argument
    {
        $$.elements.pushBack($1.element);
    }
    | /*empty*/
    ;

function_argument
    : function_argument_flag type_declaration TOKEN_IDENT
    {
        $$ = ParsingNode(context.alloc<Element>($3.location, ElementType::FunctionArgument, $3.stringData, $1.flags));
        $$.element->typeRef = $2.typeRef;
    }
    ;
	
function_argument_flag
    : TOKEN_IN { $$.flags = ElementFlag::In; };
    | TOKEN_OUT { $$.flags = ElementFlag::Out; };
    | TOKEN_INOUT { $$.flags = ElementFlag::InOut; };
	| /* empty */
	;

function_code
    : '{' { tokens.extractInnerTokenStream('}', $$.tokens); yyclearin; }
    ;

//---

type_declaration
    : type_declaration '[' TOKEN_INT_NUMBER ']' { $$ = $1; $$.typeRef->arraySizes.pushBack((int)$3.intData); }
    | type_declaration '['  ']' { $$ = $1; $$.typeRef->arraySizes.pushBack(-1); }
    | type_declaration_simple { $$ = $1; }
    ;

type_declaration_simple
    : TOKEN_IDENT
    {
        $$ = ParsingNode(context.alloc<TypeReference>($1.location, $1.stringData));
    }
    | program_type '<' TOKEN_IDENT '>'
    {
        $$ = ParsingNode(context.alloc<TypeReference>($1.location, $1.stringData));
        $$.typeRef->innerType = $3.stringData;
    }    
    ;

//---

optional_semicolon
    : ';'
    | /*empty*/
    ;

optional_comma
    : ','
    | /*empty*/
    ;

%%
