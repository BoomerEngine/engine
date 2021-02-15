%{

// [# filter: compiler\parser #]

#include "build.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderFileParserHelper.h"

#ifdef PLATFORM_MSVC
	#pragma warning ( disable: 4702 ) //: unreachable code
#endif

using ParsingNode = rendering::compiler::parser::ParsingNode;
using Element = rendering::compiler::parser::Element;
using ElementType = rendering::compiler::parser::ElementType;
using TypeReference = rendering::compiler::parser::TypeReference;
using ElementFlags = rendering::compiler::parser::ElementFlags;
using ElementFlag = rendering::compiler::parser::ElementFlag;
using CodeNode = rendering::compiler::CodeNode;
using DataType = rendering::compiler::DataType;
using DataValue = rendering::compiler::DataValue;
using OpCode = rendering::compiler::OpCode;

static int cslc_lex(rendering::compiler::parser::CodeParsingNode* outNode, rendering::compiler::parser::ParsingCodeContext& ctx, rendering::compiler::parser::ParserCodeTokenStream& tokens)
{
    return tokens.readToken(*outNode);
}

static int cslc_error(rendering::compiler::parser::ParsingCodeContext& ctx, rendering::compiler::parser::ParserCodeTokenStream& tokens, const char* txt)
{
    ctx.reportError(tokens.location(), base::TempString("parser error: {} near '{}'", txt, tokens.text()));
    return 0;
}

%}

%require "2.7"
%defines
%define api.pure full
%define api.prefix {cslc_};
%define api.value.type {rendering::compiler::parser::CodeParsingNode}
%define parse.error verbose

%parse-param { rendering::compiler::parser::ParsingCodeContext& context }
%parse-param { rendering::compiler::parser::ParserCodeTokenStream& tokens }

%lex-param { rendering::compiler::parser::ParsingCodeContext& context }
%lex-param { rendering::compiler::parser::ParserCodeTokenStream& tokens }

%token TOKEN_EXPORT
%token TOKEN_SHADER
%token TOKEN_DESCRIPTOR
%token TOKEN_CONSTANTS
%token TOKEN_STATIC_SAMPLER
%token TOKEN_RENDER_STATES
%token TOKEN_STRUCT
%token TOKEN_THIS
%token TOKEN_VERTEX
%token TOKEN_ATTR_START
%token TOKEN_ATTR_END

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

%nonassoc NO_ELSE
%nonassoc ELSE

%expect 1

%% /* The grammar follows.  */

code
    : statement_list { context.root(context.createScope($1.m_code, false)); }
    ;

statement_list
    : statement_list statement { $$ = context.linkStatements($1.m_code, $2.m_code); }
    | /* */ { $$.m_code = nullptr; }
    ;

statement
    : block_statement
	| jump_statement
    | variable_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | ';' { $$.m_code = nullptr; }
    ;

//---

variable_statement
    : variable_type_and_flags variable_declaration_list ';'
    {
        context.m_contextType = DataType();
		context.m_contextConstVar = false;
		$$.m_code = $2.m_code;
    }
	;

variable_type_and_flags
    : TOKEN_CONST variable_type
    {
         context.m_contextConstVar = true;
         context.m_contextType = $2.m_type;
    }
    | variable_type
    {
         context.m_contextConstVar = false;
         context.m_contextType = $1.m_type;
    }
    ;

variable_type
    : TOKEN_CASTABLE_TYPE
    | TOKEN_ARRAY_TYPE
    | TOKEN_VECTOR_TYPE
    | TOKEN_MATRIX_TYPE
    | TOKEN_SHADER_TYPE
    | TOKEN_STRUCT_TYPE
    ;

variable_declaration_list
    : variable_declaration_list ',' variable_declaration { $$ = context.linkStatements($1.m_code, $3.m_code); }
    | variable_declaration { $$ = $1.m_code; }
    ;

variable_declaration
    : TOKEN_IDENT variable_init
    {
		//TRACE_INFO("Variable decl {}", $1.m_string);
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::VariableDecl);
        $$.m_code->extraData().m_name = base::StringID($1.m_string);
        $$.m_code->extraData().m_castType = context.m_contextType;
		$$.m_code->extraData().m_rawAttribute = context.m_contextConstVar ? 1 : 0;

        if ($2.m_code)
            $$.m_code->addChild($2.m_code);
    }
    ;

variable_init
    : '=' assignment_expression { $$.m_code = $2.m_code; }
    | /* empty */ { $$.m_code = nullptr; }
    ;

    
//---

iteration_statement
	: statement_attributes iteration_statement_inner { 
		$$ = context.extractAttributes($1.m_code, $2.m_code);
	}
	;

iteration_statement_inner
	: TOKEN_WHILE '(' expression ')' statement
	{
         // create the loop
         $$ = context.alloc<CodeNode>($1.m_location, OpCode::Loop);
         $$.m_code->extraData().m_rawAttribute = 1; // WHILE LOOP

		 $$.m_code->addChild(nullptr); // no init
         $$.m_code->addChild($3.m_code);  // CONDITON
         $$.m_code->addChild(nullptr); // no increment
         $$.m_code->addChild($5.m_code); // BODY
	}
	| TOKEN_DO statement TOKEN_WHILE '(' expression ')' ';'
	{
		 // create the loop
         $$ = context.alloc<CodeNode>($1.m_location, OpCode::Loop);
         $$.m_code->extraData().m_rawAttribute = 2; // DO WHITE LOOP

		 $$.m_code->addChild(nullptr); // no init
         $$.m_code->addChild($5.m_code); // CONDITION
         $$.m_code->addChild(nullptr); // no increment
         $$.m_code->addChild($2.m_code); // BODY
	}
	| TOKEN_FOR '(' for_init_statement for_check_statement for_continue_statement ')' statement
	{	
		 // create loop node
		 auto loopScope = context.alloc<CodeNode>($1.m_location, OpCode::Loop);
         loopScope->extraData().m_rawAttribute = 0; // FOR LOOP

		 // loop scope contains 3 children: condition, increment and the statement
		 loopScope->addChild($3.m_code);
    	 loopScope->addChild($4.m_code);
    	 loopScope->addChild($5.m_code);
    	 loopScope->addChild($7.m_code);

    	 // create the loop scope
		 $$ = context.alloc<CodeNode>($1.m_location, OpCode::Scope);
		 $$.m_code->addChild(loopScope);
    }
	;
    
for_init_statement
    : variable_statement
    | expression_statement
    | ';' { $$.m_code = nullptr; }
    ;

for_check_statement
    : expression_statement
    | ';' { $$.m_code = context.alloc<CodeNode>($1.m_location, DataType::BoolScalarType(), DataValue(true)); }
    ;

for_continue_statement
    : expression
    | /* empty */ { $$ = nullptr; }
    ;

//---

statement_attributes
	: TOKEN_ATTR_START statement_attributes_list TOKEN_ATTR_END { $$ = $2.m_code; }
	| /* empty */ { $$ = nullptr; }
	;

statement_attributes_list
	: statement_attributes_list ',' statement_attribute	{ $$ = context.createExpressionList($1.m_code, $3.m_code); }
	| statement_attribute
	;

statement_attribute
	: TOKEN_IDENT { 
		$$.m_code = context.alloc<CodeNode>($1.m_location, OpCode::ListElement);
		auto& entry = $$.m_code->extraData().m_attributesMap.emplaceBack();
		entry.key = $1.m_string;
		}
	| TOKEN_IDENT '(' statement_attribute_value ')' {
		$$.m_code = context.alloc<CodeNode>($1.m_location, OpCode::ListElement);
		auto& entry = $$.m_code->extraData().m_attributesMap.emplaceBack();
		entry.key = $1.m_string;
		entry.value = $3.m_string;
	}
	;

statement_attribute_value
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

//---

expression_statement
    : expression ';'
    ;

expression
	: assignment_expression
//	| expression ',' assignment_expression { $$ = context.createExpressionList($1.m_code, $3.m_code); }
	;

program_instance_expression
    : TOKEN_SHADER_TYPE '{' program_instance_param_list '}'
    {
        ASSERT($1.m_type.isProgram());
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::ProgramInstance, $1.m_type);
		context.extractChildrenFromExpressionList($$.m_code, $3.m_code);
    }

    | TOKEN_SHADER_TYPE '{' '}'
    {
        ASSERT($1.m_type.isProgram());
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::ProgramInstance, $1.m_type);
    }
    ;

program_instance_param_list
    : program_instance_param_list ',' program_instance_param { $$ = context.createExpressionList($1.m_code, $3.m_code); }
    | program_instance_param
    ;

program_instance_param
    : TOKEN_IDENT '=' assignment_expression
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::ProgramInstanceParam, $3.m_code);
        $$.m_code->extraData().m_name = base::StringID($1.m_string);
    }
    ;

array_build_expression
    : TOKEN_ARRAY_TYPE '{' argument_expression_list '}'
    {
        //TRACE_ERROR("Building array {} with {} children", $1.m_type, $3.m_code->getChildren().size());
        $$ = context.createFunctionCall($2.m_location, "__create_array");
		context.extractChildrenFromExpressionList($$.m_code, $3.m_code);		
        $$.m_code->extraData().m_castType = $1.m_type;
    }

    | TOKEN_ARRAY_TYPE '{' '}'
    {
        //TRACE_ERROR("Building array {}", $1.m_type);
        $$ = context.createFunctionCall($2.m_location, "__create_array");
        $$.m_code->extraData().m_castType = $1.m_type;
    }
    ;

vector_build_expression
    : TOKEN_VECTOR_TYPE '(' argument_expression_list ')'
    {
		$$ = context.createFunctionCall($2.m_location, "__create_vector");
		context.extractChildrenFromExpressionList($$.m_code, $3.m_code);
        $$.m_code->extraData().m_castType = $1.m_type;
    }
    ;

matrix_build_expression
    : TOKEN_MATRIX_TYPE '(' argument_expression_list ')'
    {
		$$ = context.createFunctionCall($2.m_location, "__create_matrix");
		context.extractChildrenFromExpressionList($$.m_code, $3.m_code);
        $$.m_code->extraData().m_castType = $1.m_type;
    }
    ;

primary_expression
	: TOKEN_IDENT
	{
	    $$ = context.alloc<CodeNode>($1.m_location, OpCode::Ident);
	    $$.m_code->extraData().m_name = base::StringID($1.m_string);
    }
    | TOKEN_THIS
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::This);
    }
	| constant
	| '(' expression ')' { $$ = $2; }
	| vector_build_expression
	| matrix_build_expression
	| array_build_expression
	| program_instance_expression
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
    {
        $$ = context.alloc<CodeNode>($3.m_location, OpCode::AccessArray, $1.m_code, $3.m_code);
    }
	| postfix_expression '(' ')'
    {
        $$ = context.alloc<CodeNode>($3.m_location, OpCode::Call, $1.m_code);
    }
	| postfix_expression '(' argument_expression_list ')'
	{
	    $$ = context.alloc<CodeNode>($3.m_location, OpCode::Call, $1.m_code);
		context.extractChildrenFromExpressionList($$.m_code, $3.m_code);
	}
	| postfix_expression '.' ident_like
	{
	    $$ = context.alloc<CodeNode>($3.m_location, OpCode::AccessMember, $1.m_code);
	    $$.m_code->extraData().m_name = base::StringID($3.m_string);
	    ASSERT(!$$.m_code->extraData().m_name.empty());
    }
	| postfix_expression TOKEN_INC_OP { $$ = context.createFunctionCall($2.m_location, "__postInc"); }
	| postfix_expression TOKEN_DEC_OP { $$ = context.createFunctionCall($2.m_location, "__postDec"); }
	;

ident_like
    : TOKEN_IDENT
    | TOKEN_VERTEX
    | TOKEN_SHADER
    ;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression { $$ = context.createExpressionList($1.m_code, $3.m_code); }
	;

unary_expression
	: postfix_expression
	| TOKEN_INC_OP unary_expression	{ $$ = context.createFunctionCall($2.m_location, "__preInc", $2.m_code); }
	| TOKEN_DEC_OP unary_expression	{ $$ = context.createFunctionCall($2.m_location, "__preDec", $2.m_code); }
	| unary_operator cast_expression { $$ = context.createFunctionCall($2.m_location, $1.m_string, $2.m_code); }
	;

unary_operator
	: '+' { $$.m_string = ""; }
	| '-' { $$.m_string = "__neg"; }
	| '~' { $$.m_string = "__not"; }
	| '!' { $$.m_string = "__logicalNot"; }
	;

cast_expression
	: unary_expression
	| '(' cast_type ')' cast_expression
	{
        $$ = context.alloc<CodeNode>($2.m_location, OpCode::Cast, $4.m_code);
        $$.m_code->extraData().m_castType = $2.m_type;
    }
	;

cast_type
	: TOKEN_CASTABLE_TYPE
	| TOKEN_VECTOR_TYPE
	;

multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression	{ $$ = context.createFunctionCall($2.m_location, "__mul", $1.m_code, $3.m_code); }
	| multiplicative_expression '/' cast_expression	{ $$ = context.createFunctionCall($2.m_location, "__div", $1.m_code, $3.m_code); }
	| multiplicative_expression '%' cast_expression	{ $$ = context.createFunctionCall($2.m_location, "__mod", $1.m_code, $3.m_code); }
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression	{ $$ = context.createFunctionCall($2.m_location, "__add", $1.m_code, $3.m_code); }
	| additive_expression '-' multiplicative_expression	{ $$ = context.createFunctionCall($2.m_location, "__sub", $1.m_code, $3.m_code); }
	;

shift_expression
	: additive_expression
	| shift_expression TOKEN_LEFT_OP additive_expression { $$ = context.createFunctionCall($2.m_location, "__shl", $1.m_code, $3.m_code); }
	| shift_expression TOKEN_RIGHT_OP additive_expression { $$ = context.createFunctionCall($2.m_location, "__shr", $1.m_code, $3.m_code); }
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression { $$ = context.createFunctionCall($2.m_location, "__lt", $1.m_code, $3.m_code); }
	| relational_expression '>' shift_expression { $$ = context.createFunctionCall($2.m_location, "__gt", $1.m_code, $3.m_code); }
	| relational_expression TOKEN_LE_OP shift_expression { $$ = context.createFunctionCall($2.m_location, "__le", $1.m_code, $3.m_code); }
	| relational_expression TOKEN_GE_OP shift_expression { $$ = context.createFunctionCall($2.m_location, "__ge", $1.m_code, $3.m_code); }
	;

equality_expression
	: relational_expression
	| equality_expression TOKEN_EQ_OP relational_expression { $$ = context.createFunctionCall($2.m_location, "__eq", $1.m_code, $3.m_code); }
	| equality_expression TOKEN_NE_OP relational_expression { $$ = context.createFunctionCall($2.m_location, "__neq", $1.m_code, $3.m_code); }
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression { $$ = context.createFunctionCall($2.m_location, "__and", $1.m_code, $3.m_code); }
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression { $$ = context.createFunctionCall($2.m_location, "__xor", $1.m_code, $3.m_code); }
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression { $$ = context.createFunctionCall($2.m_location, "__or", $1.m_code, $3.m_code); }
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression TOKEN_AND_OP inclusive_or_expression { $$ = context.createFunctionCall($2.m_location, "__logicAnd", $1.m_code, $3.m_code); }
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression TOKEN_OR_OP logical_and_expression { $$ = context.createFunctionCall($2.m_location, "__logicOr", $1.m_code, $3.m_code); }
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression { $$ = context.createFunctionCall($2.m_location, "__select", $1.m_code, $3.m_code, $5.m_code); }
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression { $$ = context.createFunctionCall($2.m_location, $2.m_string, $1.m_code, $3.m_code); }
	;

assignment_operator
	: '=' { $$.m_string = "__assign"; }
	| TOKEN_MUL_ASSIGN { $$.m_string = "__assign__mul"; }
	| TOKEN_DIV_ASSIGN { $$.m_string = "__assign__div"; }
	| TOKEN_MOD_ASSIGN { $$.m_string = "__assign__mod"; }
	| TOKEN_ADD_ASSIGN { $$.m_string = "__assign__add"; }
	| TOKEN_SUB_ASSIGN { $$.m_string = "__assign__sub"; }
	| TOKEN_LEFT_ASSIGN { $$.m_string = "__assign__shl"; }
	| TOKEN_RIGHT_ASSIGN { $$.m_string = "__assign__shr"; }
	| TOKEN_AND_ASSIGN { $$.m_string = "__assign__and"; }
	| TOKEN_XOR_ASSIGN { $$.m_string = "__assign__xor"; }
	| TOKEN_OR_ASSIGN { $$.m_string = "__assign__or"; }
	;

//---

block_statement
    : '{' statement_list '}' { $$ = context.createScope($2.m_code); }
    ;

//---

jump_statement
    : TOKEN_BREAK ';'
    {
       $$ = context.alloc<CodeNode>($1.m_location, OpCode::Break);
    }

    | TOKEN_CONTINUE ';'
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::Continue);
    }

    | TOKEN_RETURN expression ';'
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::Return, $2.m_code);
    }

    | TOKEN_RETURN ';'
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::Return);
    }

    | TOKEN_DISCARD ';'
    {
        $$ = context.alloc<CodeNode>($1.m_location, OpCode::Exit);
    }
    ;

//---

selection_statement
	: statement_attributes selection_statement_inner {
		$$ = context.extractAttributes($1.m_code, $2.m_code);
	}
	;

selection_statement_inner
	: TOKEN_IF '(' expression ')' statement %prec NO_ELSE
	{
	    $$ = context.alloc<CodeNode>($1.m_location, OpCode::IfElse, $3.m_code, $5.m_code);
	}
	| TOKEN_IF '(' expression ')' statement TOKEN_ELSE statement %prec ELSE
	{
	    $$ = context.alloc<CodeNode>($1.m_location, OpCode::IfElse, $3.m_code, $5.m_code, $7.m_code);
	}
	;

//---

constant
    : TOKEN_INT_NUMBER
    | TOKEN_FLOAT_NUMBER
    | TOKEN_BOOL_TRUE
    | TOKEN_BOOL_FALSE
	| TOKEN_NAME
    ;

%%
