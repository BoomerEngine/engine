%{

#include "build.h"
#include "core/script_compiler/src/scriptFunctionCodeParserHelper.h"

#ifdef PLATFORM_MSVC
	#pragma warning ( disable: 4702 ) //: unreachable code
#endif

using FunctionParsingNode = boomer::script::FunctionParsingNode;
using Op = boomer::script::FunctionNodeOp;

static int bfc_lex(boomer::script::FunctionParsingNode* outNode, boomer::script::FunctionParsingContext& ctx, boomer::script::FunctionParsingTokenStream& tokens)
{
    return tokens.readToken(*outNode);
}

static int bfc_error(boomer::script::FunctionParsingContext& ctx, boomer::script::FunctionParsingTokenStream& tokens, const char* txt)
{
    ctx.reportError(tokens.location(), boomer::TempString("parser error: {} near '{}'", txt, tokens.text()));
    return 0;
}

%}

%require "3.0"
%defines
%define api.pure full
%define api.prefix {bfc_};
%define api.value.type {boomer::script::FunctionParsingNode}
%define parse.error verbose

%parse-param { boomer::script::FunctionParsingContext& context }
%parse-param { boomer::script::FunctionParsingTokenStream& tokens }

%lex-param { boomer::script::FunctionParsingContext& context }
%lex-param { boomer::script::FunctionParsingTokenStream& tokens }

%token TOKEN_IDENT
%token TOKEN_INT_NUMBER
%token TOKEN_UINT_NUMBER
%token TOKEN_FLOAT_NUMBER
%token TOKEN_STRING
%token TOKEN_NAME
%token TOKEN_TYPE
%token TOKEN_BOOL_TRUE
%token TOKEN_BOOL_FALSE
%token TOKEN_NULL

%token TOKEN_USING
%token TOKEN_OVERRIDE
%token TOKEN_THIS
%token TOKEN_SUPER
%token TOKEN_IN
%token TOKEN_EXTENDS
%token TOKEN_PARENT
%token TOKEN_STATE
%token TOKEN_STRUCT
%token TOKEN_CONST
%token TOKEN_CLASS
%token TOKEN_ENUM
%token TOKEN_FINAL
%token TOKEN_ARRAY
%token TOKEN_PTR
%token TOKEN_WEAK
%token TOKEN_REF
%token TOKEN_OUT
%token TOKEN_FUNCTION
%token TOKEN_SIGNAL
%token TOKEN_PROPERTY
%token TOKEN_VAR
%token TOKEN_LOCAL
%token TOKEN_PROTECTED
%token TOKEN_PRIVATE
%token TOKEN_IMPORT
%token TOKEN_STATIC
%token TOKEN_EDITABLE
%token TOKEN_INLINED
%token TOKEN_ABSTRACT
%token TOKEN_OPCODE
%token TOKEN_OPERATOR
%token TOKEN_CAST
%token TOKEN_EXPLICIT
%token TOKEN_TYPENAME
%token TOKEN_ENGINE_TYPE
%token TOKEN_TYPE_TYPE
%token TOKEN_UNSAFE
%token TOKEN_ALIAS

%token TOKEN_BREAK
%token TOKEN_CONTINUE
%token TOKEN_RETURN
%token TOKEN_DO
%token TOKEN_WHILE
%token TOKEN_IF
%token TOKEN_ELSE
%token TOKEN_FOR
%token TOKEN_CASE
%token TOKEN_SWITCH
%token TOKEN_DEFAULT
%token TOKEN_NEW

%token TOKEN_INC_OP
%token TOKEN_DEC_OP
%token TOKEN_LEFT_OP
%token TOKEN_RIGHT_OP
%token TOKEN_RIGHT_RIGHT_OP
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
%token TOKEN_AT_ASSIGN
%token TOKEN_HAT_ASSIGN
%token TOKEN_HASH_ASSIGN
%token TOKEN_DOLAR_ASSIGN
%token TOKEN_LEFT_ASSIGN
%token TOKEN_RIGHT_ASSIGN
%token TOKEN_RIGHT_RIGHT_ASSIGN
%token TOKEN_AND_ASSIGN
%token TOKEN_XOR_ASSIGN
%token TOKEN_OR_ASSIGN
 
%nonassoc NO_ELSE
%nonassoc ELSE

%expect 1

%% /* The grammar follows.  */

function_code
    : statement_list {
        $$.node = context.createNode($1.location, Op::Scope); 
        $$.node->addFromStatementList($1.node);
        context.rootStatement($$.node);
    }
    ;

statement_list
	: statement_list statement { 
        $$.node = context.createNode($1.location, Op::StatementList);
        $$.node->addFromStatementList($1.node);
        $$.node->addFromStatementList($2.node);	    
    }
	| /* empty */ { $$.node = nullptr; }
	;
	    
statement
	: compound_statement
	| empty_statement
	| expression_statement { $$.node = context.makeBreakpoint($1.node); }
	| jump_statement { $$.node = context.makeBreakpoint($1.node); }
	| selection_statement { $$.node = context.makeBreakpoint($1.node); }
	| loop_statement
	| switch_statement { $$.node = context.makeBreakpoint($1.node); }
	| labeled_statement
	| variable_statement
	| error ';' { $$.node = context.createNode($2.location, Op::Nop); }
	;

compound_statement
    : '{' statement_list '}' { 
        $$.node = context.createNode($1.location, Op::Scope);
        $$.node->addFromStatementList($2.node);
    }
    ;
    
empty_statement
	: /* empty */ ';' { $$.node = context.createNode($1.location, Op::Nop); }
	;
	
variable_statement
    : var_statement_type var_names ';' {
        $$.node = $2.node;
        context.currentType = boomer::script::FunctionTypeInfo();
    }
    ;
    
var_names
    : var_name { $$.node = $1.node; }
    | var_names ',' var_name { 
        $$.node = context.createNode($1.location, Op::StatementList);
        $$.node->addFromStatementList($1.node);
        $$.node->addFromStatementList($3.node);
    }
    ;
    
var_name
    : TOKEN_IDENT '=' assignment_expresion {
        auto* node = context.createNode($1.location, Op::Var);
        node->data.name = $1.name;
        node->data.type = context.currentType;
        node->add($3.node);
        $$.node = node;
    }
    
    | TOKEN_IDENT {
        auto* node = context.createNode($1.location, Op::Var);
        node->data.name = $1.name;
        node->data.type = context.currentType;
        $$.node = node;
    }
    ;    
    
var_statement_type
    : var_type { context.currentType = $1.type; }
    ;
    
var_type
    : var_type_inner
    | var_type '[' TOKEN_INT_NUMBER ']' { $$.type = context.createStaticArrayType($1.type, $3.intValue); }
    ;

var_type_inner
    : TOKEN_TYPE
    | TOKEN_CLASS '<' TOKEN_TYPE '>' { $$.type = context.createClassType($1.location, $3.name); }
    | TOKEN_PTR '<' TOKEN_TYPE '>' { $$.type = context.createPtrType($1.location, $3.name); }
    | TOKEN_WEAK '<' TOKEN_TYPE '>' { $$.type = context.createWeakPtrType($1.location, $3.name); }
    | TOKEN_ARRAY '<' var_type '>' { $$.type = context.createDynamicArrayType($3.type); }
    ;
    
expression_statement
	: expression ';'
	;

selection_statement
	: selection_statement_header '(' expression ')' statement   %prec NO_ELSE
	{
	    $$.node->children[0]->add($3.node);
        $$.node->children[0]->add($5.node);
    }
	| selection_statement_header '(' expression ')' statement TOKEN_ELSE statement  %prec ELSE
	{
	    $$.node->children[0]->add($3.node);
        $$.node->children[0]->add($5.node);
        $$.node->children[0]->add($7.node);
    }
	;
	
selection_statement_header
    : TOKEN_IF {
        $$.node = context.createNode($1.location, Op::Scope);
        $$.node->add(context.createNode($1.location, Op::IfThenElse));
    }
    ;

switch_statement
	: switch_header statement
	{ 
	    $$.node->children[0]->add($2.node);
	    context.popContextNode();
    }
	;

switch_header
	: switch_header_header '(' expression ')' {
         $$.node->children[0]->add($3.node);
         context.pushContextNode($$.node->children[0]);
    }
	;
	
switch_header_header
    : TOKEN_SWITCH {
        $$.node = context.createNode($1.location, Op::Scope); 
        $$.node->add(context.createNode($1.location, Op::Switch));        
    }
    ;

labeled_statement
	: TOKEN_CASE expression ':' statement {
	    auto* scope = context.findContextNode(Op::Switch);
        auto* node = context.createNode($1.location, Op::Case, $2.node, $4.node);
        node->contextNode = scope;
	    $$.node = node;
    }
    
	| TOKEN_DEFAULT ':' statement {
	    auto* scope = context.findContextNode(Op::Switch);
        auto* node = context.createNode($1.location, Op::DefaultCase, $3.node);
        node->contextNode = scope;
	    $$.node = node;
    }
	;

loop_statement
	: loop_for_header '(' for_init_statement for_check_statement for_continue_statement ')' statement {	
        $1.node->add($4.node); // condition
        $1.node->add($5.node); // increment
        $1.node->add($7.node); // body
        
	    auto* loopScope = context.createNode($2.location, Op::Scope);
        loopScope->add($3.node);
	    loopScope->add($1.node);	    
		$$.node = loopScope; 
		
        context.popContextNode();
    }

	| loop_while_header '(' assignment_expresion ')' statement {	
        $$.node->add(context.makeBreakpoint($3.node)); // condition
        $$.node->add(nullptr); // increment
        $$.node->add($5.node); // body        
        context.popContextNode();
    }

	| loop_do_header statement TOKEN_WHILE '(' assignment_expresion ')' {	    
        $$.node->add(context.makeBreakpoint($5.node)); // condition
        $$.node->add(nullptr); // increment
        $$.node->add($2.node); // body	
        context.popContextNode();
    }
	;
	
loop_for_header
    : TOKEN_FOR { 
        $$.node = context.createNode($1.location, Op::For);
        context.pushContextNode($$.node);
    }
    ;	
    
loop_while_header
    : TOKEN_WHILE { 
        $$.node = context.createNode($1.location, Op::While);	
        context.pushContextNode($$.node);
    }
    ;
    
loop_do_header
    : TOKEN_DO { 
        $$.node = context.createNode($1.location, Op::DoWhile);
        context.pushContextNode($$.node);
    }
    ;    
    
for_init_statement
    : variable_statement
    | expression_statement
    | ';' { $$.node = nullptr; }
    ;

for_check_statement
    : expression_statement
    | ';' { $$.node = context.createBoolConst($1.location, true); }
    ;

for_continue_statement
    : expression
    | /* empty */ { $$.node = nullptr; }
    ;    

jump_statement
	: TOKEN_RETURN ';' {
        $$.node = context.createNode($1.location, Op::Return, nullptr);
    }
    
    | TOKEN_RETURN expression ';' {
        $$.node = context.createNode($1.location, Op::Return, $2.node);	    
    }
    
	| TOKEN_BREAK ';'
	{
        $$.node = context.createNode($1.location, Op::Break);
        $$.node->contextNode = context.findBreakContextNode();
    }
        
	| TOKEN_CONTINUE ';' {
        $$.node = context.createNode($1.location, Op::Continue);
        $$.node->contextNode = context.findContinueContextNode();	
    }
	;


expression
	: assignment_expresion
	| expression ',' assignment_expresion {
    	$$.node = context.createNode($2.location, Op::ExpressionList);
    	$$.node->addFromExpressionList($1.node);
        $$.node->addFromExpressionList($3.node);
	}
	;

assignment_expresion 
	: conditional_expression { $$.node = $1.node; }
	
	| conditional_expression '=' assignment_expresion {
	    $$.node = context.createNode($2.location, Op::Assign, $1.node, $3.node);
    }
    
	| conditional_expression assignment_operator assignment_expresion {
        $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = $2.name;
    }
	;

assignment_operator  
    : TOKEN_MUL_ASSIGN { $$.location = $1.location; $$.name = "opMulAssign"_id; }
    | TOKEN_DIV_ASSIGN { $$.location = $1.location; $$.name = "opDivAssign"_id; } 
    | TOKEN_MOD_ASSIGN { $$.location = $1.location; $$.name = "opModAssign"_id; } 
    | TOKEN_ADD_ASSIGN { $$.location = $1.location; $$.name = "opAddAssign"_id; } 
    | TOKEN_SUB_ASSIGN { $$.location = $1.location; $$.name = "opSubAssign"_id; } 
    | TOKEN_AT_ASSIGN { $$.location = $1.location; $$.name = "opAtAssign"_id; } 
    | TOKEN_HAT_ASSIGN { $$.location = $1.location; $$.name = "opHatAssign"_id; } 
    | TOKEN_HASH_ASSIGN { $$.location = $1.location; $$.name = "opHashAssign"_id; } 
    | TOKEN_DOLAR_ASSIGN { $$.location = $1.location; $$.name = "opDolarAssign"_id; } 
    | TOKEN_LEFT_ASSIGN { $$.location = $1.location; $$.name = "opShiftLeftAssign"_id; } 
    | TOKEN_RIGHT_ASSIGN { $$.location = $1.location; $$.name = "opShiftRightAssign"_id; } 
    | TOKEN_RIGHT_RIGHT_ASSIGN { $$.location = $1.location; $$.name = "opShiftRightUnsignedAssign"_id; } 
    | TOKEN_AND_ASSIGN { $$.location = $1.location; $$.name = "opBinaryAndAssign"_id; } 
    | TOKEN_OR_ASSIGN { $$.location = $1.location; $$.name = "opBinaryOrAssign"_id; } 
	;

conditional_expression
	: logical_or_expression { $$.node = $1.node; }
	
	| logical_or_expression '?' expression ':' conditional_expression {
	    $$.node = context.createNode($2.location, Op::Conditional, $1.node, $3.node, $5.node);
    }
	;

logical_or_expression
	: logical_and_expression { $$.node = $1.node; }
	
	| logical_or_expression TOKEN_OR_OP logical_and_expression {
	    $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = "opLogicOr"_id;
    }
	;

logical_and_expression
	: inclusive_or_expression { $$.node = $1.node; }
	
	| logical_and_expression TOKEN_AND_OP inclusive_or_expression {
	    $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = "opLogicAnd"_id;
    }
	;

inclusive_or_expression
	: exclusive_or_expression { $$.node = $1.node; }
	
	| inclusive_or_expression '|' exclusive_or_expression {
	    $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = "opBinaryOr"_id;
    }
	;

exclusive_or_expression
	: and_expression { $$.node = $1.node; }
	
	| exclusive_or_expression '^' and_expression {
	    $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = "opHat"_id;
    }
	;

and_expression
	: equality_expression { $$.node = $1.node; }
	
	| and_expression '&' equality_expression {
	    $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = "opBinaryAnd"_id;
    }
	;

equality_expression
	: relational_expression { $$.node = $1.node; }
	
	| equality_expression equality_operator relational_expression {
        $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = $2.name;
    }
	;

equality_operator
	: TOKEN_EQ_OP { $$.location = $1.location; $$.name = "opEqual"_id; }
	| TOKEN_NE_OP { $$.location = $1.location; $$.name = "opNotEqual"_id; }
	;

relational_expression
	: addtive_expression { $$.node = $1.node; }
	| relational_expression relational_operator addtive_expression {
        $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = $2.name;
    }
	;

relational_operator
	: '<' { $$.location = $1.location; $$.name = "opLess"_id; }
	| '>' { $$.location = $1.location; $$.name = "opGreater"_id; }
	| TOKEN_LE_OP { $$.location = $1.location; $$.name = "opLessEqual"_id; }
	| TOKEN_GE_OP { $$.location = $1.location; $$.name = "opGreaterEqual"_id; }
	;

addtive_expression
	: multiplicative_expression { $$.node = $1.node; }
	| addtive_expression addtive_operartor multiplicative_expression {
        $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = $2.name;
    }
	;
	
addtive_operartor
    : '+' { $$.location = $1.location; $$.name = "opAdd"_id; }
    | '-' { $$.location = $1.location; $$.name = "opSubtract"_id; }	
    ;

multiplicative_expression
	: cast_expression { $$.node = $1.node; }
	
	| multiplicative_expression multiplicative_operartor cast_expression {
        $$.node = context.createNode($2.location, Op::Operator, $1.node, $3.node);
        $$.node->data.name = $2.name;
    }
	;
	
multiplicative_operartor
    : '*' { $$.location = $1.location; $$.name = "opMultiply"_id; }
    | '/' { $$.location = $1.location; $$.name = "opDivide"_id; }
    | '%' { $$.location = $1.location; $$.name = "opModulo"_id; }		
    ;	

cast_expression
	: unary_expression { $$.node = $1.node; } 
	;

unary_expression
	: allocation_expression
	| unary_operator unary_expression {
        $$.node = context.createNode($1.location, Op::Operator, $2.node);
        $$.node->data.name = $1.name;
    }
	;
	
unary_operator
    : '-'  { $$.location = $1.location; $$.name = "opNegate"_id; }
    | '!'  { $$.location = $1.location; $$.name = "opNot"_id; }
    | '~'  { $$.location = $1.location; $$.name = "opBinaryNot"_id; }
    | TOKEN_INC_OP { $$.location = $1.location; $$.name = "opIncrement"_id; }
	| TOKEN_DEC_OP { $$.location = $1.location; $$.name = "opDecrement"_id; }
    ;

allocation_expression
	: TOKEN_NEW allocation_type TOKEN_IN postfix_expression {
	    $$.node = context.createNode($1.location, Op::New, $2.node, $4.node);
    }
	| TOKEN_NEW allocation_type {
	    $$.node = context.createNode($1.location, Op::New, $2.node, nullptr);
    }
    | postfix_expression
	;
	
allocation_type
    : postfix_expression { $$.node = $1.node; }
    ;	

postfix_expression
	: primary_expression { $$.node = $1.node; } 
	
	| postfix_expression '[' expression ']'
	{
	    $$.node = context.createNode($2.location, Op::AccessIndex, $1.node, $3.node);
    }
    
    | postfix_expression '(' ')' {
    	$$.node = context.createNode($1.location, Op::Call, $1.node);
    }
    
	| postfix_expression '(' func_param_list ')' {
    	$$.node = context.createNode($1.location, Op::Call, $1.node);
    	for (auto* ptr : $3.nodes)
            $$.node->add(ptr);
    }
    
    | postfix_cast_type '(' func_param_list ')' { 
        auto* typeNode = context.createNode($1.location, Op::Type);
        typeNode->data.type = $1.type;
	    typeNode->data.name = $1.name;
	    
      	$$.node = context.createNode($1.location, Op::Call, typeNode);
    	for (auto* ptr : $3.nodes)
            $$.node->add(ptr);
    }
    
    | postfix_cast_type '(' ')' { 
        auto* typeNode = context.createNode($1.location, Op::Type);
        typeNode->data.type = $1.type;
	    typeNode->data.name = $1.name;	    
      	$$.node = context.createNode($1.location, Op::Call, typeNode);    	
    }
    
	| postfix_expression '.' TOKEN_IDENT { 
	    $$.node = context.createNode($3.location, Op::AccessMember, $1.node);
        $$.node->data.name = $3.name;
    }
    
    | postfix_type '.' TOKEN_IDENT {
       auto* typeNode = context.createNode($1.location, Op::Type);
       typeNode->data.type = $1.type;
       typeNode->data.name = $1.name;	    
   
       $$.node = context.createNode($3.location, Op::AccessMember, typeNode);
       $$.node->data.name = $3.name;
    }
    
    | postfix_expression TOKEN_INC_OP {
        $$.node = context.createNode($2.location, Op::Operator, $1.node);
        $$.node->data.name = "opPostIncrement"_id;
    }
        
	| postfix_expression TOKEN_DEC_OP {
        $$.node = context.createNode($2.location, Op::Operator, $1.node);
        $$.node->data.name = "opPostDecrement"_id;
    }
	;
	
postfix_type
    : TOKEN_TYPE    
    ;
    
postfix_cast_type
    : TOKEN_TYPE 
    | TOKEN_CLASS '<' TOKEN_TYPE '>' { $$.type = context.createClassType($1.location, $3.name); }
    | TOKEN_PTR '<' TOKEN_TYPE '>' { $$.type = context.createPtrType($1.location, $3.name); }
    | TOKEN_WEAK '<' TOKEN_TYPE '>' { $$.type = context.createWeakPtrType($1.location, $3.name); }   
    ;

func_param_list
	: func_param_list ',' func_param { $$.nodes.pushBack($3.node); }
	| func_param { $$.nodes.clear(); $$.nodes.pushBack($1.node); }
	;

func_param
	: conditional_expression
	;

primary_expression
	: TOKEN_IDENT { 
	    $$.node = context.createNode($1.location, Op::Ident);
	    $$.node->data.name = $1.name;
    }
    
	| TOKEN_THIS { $$.node = context.createNode($1.location, Op::This); }
	
	| primary_const
		
	| '(' expression ')' { $$.node = $2.node; }
	;

primary_const
	: TOKEN_INT_NUMBER { $$.node = context.createIntConst($1.location, $1.intValue); }    
	| TOKEN_UINT_NUMBER { $$.node = context.createUintConst($1.location, $1.uintValue); }	
	| TOKEN_FLOAT_NUMBER { $$.node = context.createFloatConst($1.location, $1.floatValue); }	
	| TOKEN_NAME { $$.node = context.createNameConst($1.location, $1.name); }
	| TOKEN_STRING { $$.node = context.createStringConst($1.location, $1.stringValue); }
    | TOKEN_CLASS TOKEN_NAME { $$.node = context.createClassTypeConst($1.location, $2.name); }		
    | TOKEN_BOOL_TRUE { $$.node = context.createBoolConst($1.location, true); }	
	| TOKEN_BOOL_FALSE { $$.node = context.createBoolConst($1.location, false); }
	| TOKEN_NULL { $$.node = context.createNullConst($1.location); } 
	;
   
%%

