%{

#include "build.h"
#include "core/script_compiler/src/scriptFileParserHelper.h"

#ifdef PLATFORM_MSVC
    #pragma warning ( disable: 4702 ) //: unreachable code
#endif

using FileParsingNode = boomer::script::FileParsingNode;
using StubFlag = boomer::script::StubFlag;
using StubFlags = boomer::script::StubFlags;

static int bsc_lex(boomer::script::FileParsingNode* outNode, boomer::script::FileParsingContext& ctx, boomer::script::FileParsingTokenStream& tokens)
{
    return tokens.readToken(*outNode);
}

static int bsc_error(boomer::script::FileParsingContext& ctx, boomer::script::FileParsingTokenStream& tokens, const char* txt)
{
    ctx.reportError(tokens.location(), boomer::TempString("parser error: {} near '{}'", txt, tokens.text()));
    return 0;
}

%}

%require "3.0"
%defines
%define api.pure full
%define api.prefix {bsc_};
%define api.value.type {boomer::script::FileParsingNode}
%define parse.error verbose

%parse-param { boomer::script::FileParsingContext& context }
%parse-param { boomer::script::FileParsingTokenStream& tokens }

%lex-param { boomer::script::FileParsingContext& context }
%lex-param { boomer::script::FileParsingTokenStream& tokens }

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
 
%% /* The grammar follows.  */

file
    : global_declaration_list { }
    ;

global_declaration_list
    : global_declaration_list global_declaration  {  }
    | /* empty */ {  }
    ;

global_declaration
    : decl_global_flags struct_definition
    | decl_global_flags class_declaration
    | decl_global_flags enum_declaration
    | decl_global_flags function_declaration
    | typename_declaration
    | const_declaration
    | using_declaration
    ;

//---

using_declaration
    : TOKEN_USING TOKEN_IDENT ';' {
          context.createModuleImport($1.location, $2.name);
    }
    ;
    
//---

typename_declaration
    : TOKEN_TYPENAME TOKEN_IDENT '=' type_declaration ';' {
        context.createTypeName($2.location, $2.name, $4.typeDecl);
    }
    ;
    
//---

const_declaration
    : TOKEN_CONST type_declaration TOKEN_IDENT '=' const_value ';' {        
        auto* stub = context.addConstant($3.location, $3.name);
        stub->typeDecl = $2.typeDecl;
        stub->value = $5.constValue;
        context.currentConstantValue.clear();
    }
    ;
    
const_value
    : const_value_compound const_value_list_container ')' {
        $$.constValue = $1.constValue;
        if (!context.currentConstantValue.empty())
            context.currentConstantValue.popBack();
    }
    | const_value_simple { $$.constValue = $1.constValue; }
    ;

const_value_compound
    : TOKEN_IDENT '(' {
        auto typeRef = context.createTypeRef($1.location, $1.name);
        auto typeDecl = context.createSimpleType($1.location,typeRef);
        $$.constValue = context.createConstValueCompound($1.location, typeDecl);
        context.currentConstantValue.pushBack($$.constValue);
    }
    ;

const_value_list_container
    : const_value_list
    | /* empty */
    ;
    
const_value_list
    : const_value_list ',' const_value {
      if (nullptr != $3.constValue) {
        if (!context.currentConstantValue.empty())
            context.currentConstantValue.back()->compoundVals.pushBack($3.constValue);
      }
    }
    | const_value {
      if (nullptr != $1.constValue) {
        if (!context.currentConstantValue.empty())
            context.currentConstantValue.back()->compoundVals.pushBack($1.constValue);
      }
    }
    ;            
            
const_value_simple
    : TOKEN_INT_NUMBER { $$.constValue = context.createConstValueInt($1.location, $1.intValue); }
    | '-' TOKEN_INT_NUMBER { $$.constValue = context.createConstValueInt($2.location, -$2.intValue); }
    | TOKEN_UINT_NUMBER { $$.constValue = context.createConstValueUint($1.location, $1.uintValue); }
    | TOKEN_FLOAT_NUMBER { $$.constValue = context.createConstValueFloat($1.location, $1.floatValue); }
    | '-' TOKEN_FLOAT_NUMBER { $$.constValue = context.createConstValueFloat($2.location, -$2.floatValue); }
    | TOKEN_BOOL_TRUE { $$.constValue = context.createConstValueBool($1.location, true); }
    | TOKEN_BOOL_FALSE { $$.constValue = context.createConstValueBool($1.location, false); }
    | TOKEN_STRING { $$.constValue = context.createConstValueString($1.location, $1.stringValue); }
    | TOKEN_NAME { $$.constValue = context.createConstValueName($1.location, $1.name); }
    ;

//---

struct_definition
    : struct_header struct_element_list '}' optional_semicolon {
        context.endObject();
    }
    ;
    
struct_header
    : TOKEN_STRUCT TOKEN_IDENT struct_alias '{' {
        auto flags = context.globalFlags | StubFlag::Struct;
        auto* classType = context.beginCompound($2.location, $2.name, flags);
        classType->engineImportName = $3.name;
    }
    ;
    
struct_alias
    : TOKEN_ALIAS TOKEN_NAME { $$.name = $2.name; }
    | /* empty */ { $$.name = boomer::StringID(); }
    ;    

struct_element_list
    : struct_element_list struct_element {} 
    | /* empty */
    ;

struct_element
    : decl_global_flags struct_var
    | decl_global_flags function_declaration    
    | decl_global_flags struct_definition
    | decl_global_flags const_declaration
    ;
    
struct_var    
    : TOKEN_VAR type_declaration TOKEN_IDENT struct_element_initialization ';' {
        auto flags = context.globalFlags;
        auto* var = context.addVar($3.location, $3.name, $2.typeDecl, flags);
        var->defaultValue = $4.constValue;
    }
    ;

struct_element_initialization
    : '=' const_value { $$.constValue = $2.constValue; }
    | { $$.constValue = nullptr; } /* empty */
    ;

//---

enum_declaration
    : enum_header enum_element_list optional_comma '}' optional_semicolon {
        context.endObject();
    } 
    ; 
    
enum_header
    : TOKEN_ENUM TOKEN_IDENT enum_alias '{' {
        auto flags = context.globalFlags;
        auto* stub = context.beginEnum($2.location, $2.name, flags);
        stub->engineImportName = $3.name;
    }
    ;
    
enum_alias
    : TOKEN_ALIAS TOKEN_NAME { $$.name = $2.name; }
    | /* empty */ { $$.name = boomer::StringID(); }
    ;            
    
enum_element_list
    : enum_element_list ',' enum_element
    | enum_element
    ;

enum_element
    : TOKEN_IDENT {
         context.addEnumOption($1.location, $1.name);
    }
    | TOKEN_IDENT '=' TOKEN_INT_NUMBER { 
        context.addEnumOption($1.location, $1.name, true, $3.intValue);
    }
    | TOKEN_IDENT '=' '-' TOKEN_INT_NUMBER { 
        context.addEnumOption($1.location, $1.name, true, -$4.intValue);
    }
    | TOKEN_IDENT '=' TOKEN_UINT_NUMBER { 
        context.addEnumOption($1.location, $1.name, true, $3.uintValue);
    }
    ;

//---

class_declaration
    : class_header class_element_list '}' optional_semicolon { 
        context.endObject(); 
    }
    ;
    
class_header
    : class_type TOKEN_IDENT class_alias class_extends class_in '{' {
        auto flags = $1.flags | context.globalFlags;
        auto* stub = context.beginCompound($2.location, $2.name, flags);
        stub->baseClassName = $4.name;
        stub->parentClassName = $5.name;
        stub->engineImportName = $3.name;
    }
    ;
    
class_alias
    : TOKEN_ALIAS TOKEN_NAME { $$.name = $2.name; }
    | /* empty */ { $$.name = boomer::StringID(); }
    ;  
        
class_type
    : TOKEN_CLASS { $$.flags = StubFlag::Class; }
    | TOKEN_STATE { $$.flags = StubFlag::State; }
    ;
    
class_extends
    : TOKEN_EXTENDS type_ref_name { $$.name = $2.name; }
    | /* empty */     { $$.name = boomer::StringID(); }
    ;

class_in
    : TOKEN_IN type_ref_name { $$.name = $2.name; }
    | /* empty */    { $$.name = boomer::StringID(); } 
    ;
   
class_element_list
    : class_element_list class_element {} 
    | /* empty */
    ;
    
class_element    
    : decl_global_flags class_var
    | decl_global_flags function_declaration
    | decl_global_flags struct_definition
    | decl_global_flags class_declaration    
    | decl_global_flags const_declaration
    ;
    
class_var
    : TOKEN_VAR type_declaration TOKEN_IDENT ';' {
        auto flags = context.globalFlags;
        context.addVar($3.location, $3.name, $2.typeDecl, flags);
    }
    ;
    
function_declaration
    : function_header function_args ')' function_body {
        context.currentFunction->tokens = $4.tokens; 
        context.currentFunction->flags |= $4.flags; // opcode
        context.currentFunction->opcodeName = $4.name; // opcode name
        context.currentFunction = nullptr;
    }
    ; 

function_args
    : function_arg_list
    | /* empty */
    ;
        
function_arg_list
    : function_arg_list ',' function_arg
    | function_arg
    ;
    
function_arg
    : function_arg_flags type_declaration TOKEN_IDENT function_arg_optional_default_value {
        auto flags = $1.flags;
        if (auto* var = context.addFunctionArg($3.location, $3.name, $2.typeDecl, flags))            
            var->defaultValue = $4.constValue;
    }
    ;
   
function_arg_flags
    : TOKEN_REF { $$.flags = StubFlag::Ref; }
    | TOKEN_OUT { $$.flags = StubFlag::Out; }
    | TOKEN_EXPLICIT { $$.flags = StubFlag::Explicit; }
    | /* emtpy */ { $$.flags = StubFlags(); }
    ;    
    
function_arg_optional_default_value
    : '=' const_value { $$.constValue = $2.constValue; }
    | /* empty */ { $$.constValue = nullptr; }
    ;
    
function_header
    : function_type '(' {
        auto flags = context.globalFlags | $1.flags;
        auto* func = context.addFunction($1.location, $1.name, flags);
        func->castCost = $1.intValue; // HACK, but eh, let's live with it
        func->aliasName = $1.name;
        func->returnTypeDecl = $1.typeDecl;
        context.currentFunction = func;
    }
    ;
    
function_type    
    : TOKEN_FUNCTION function_alias type_declaration TOKEN_IDENT { 
        $$.flags = StubFlag::Function; 
        $$.name = $4.name; 
        $$.location = $4.location; 
        $$.intValue = 0; 
        $$.intValue = $2.name;
        $$.typeDecl = $3.typeDecl;        
    }
    
    | TOKEN_FUNCTION function_alias TOKEN_IDENT { 
        $$.flags = StubFlag::Function; 
        $$.name = $3.name; 
        $$.location = $3.location;
        $$.intValue = 0; 
        $$.intValue = $2.name;
        $$.typeDecl = nullptr;
    }
    
    | TOKEN_SIGNAL type_declaration TOKEN_IDENT { 
        $$.flags = StubFlag::Signal; 
        $$.name = $3.name; 
        $$.location = $3.location; 
        $$.intValue = 0; 
        $$.typeDecl = $2.typeDecl;        
    }
    
    | TOKEN_SIGNAL TOKEN_IDENT {
        $$.flags = StubFlag::Signal;
        $$.name = $2.name;
        $$.location = $2.location;
        $$.intValue = 0;
        $$.typeDecl = nullptr;
    }
    
    | TOKEN_PROPERTY type_declaration TOKEN_IDENT {
        $$.flags = StubFlag::Property;
        $$.name = $3.name;
        $$.location = $3.location;
        $$.intValue = 0;
        $$.typeDecl = $2.typeDecl;
    }
    
    | TOKEN_OPERATOR function_operator_type type_declaration {
        $$.flags = StubFlag::Operator;
        $$.name = $2.name;
        $$.location = $1.location;
        $$.intValue = 0;
        $$.typeDecl = $3.typeDecl;
    }
    
    | TOKEN_CAST function_cast_cost type_declaration {
        $$.flags = StubFlag::Cast;
        $$.name = boomer::StringID("Cast");
        $$.location = $1.location;
        $$.intValue = $2.intValue;
        $$.typeDecl = $3.typeDecl;
    }
    
    | TOKEN_EXPLICIT TOKEN_CAST function_cast_cost type_declaration {
        $$.flags = StubFlags(StubFlag::Cast) | StubFlag::Explicit;
        $$.name = boomer::StringID("Cast");
        $$.location = $1.location;
        $$.intValue = $3.intValue;
        $$.typeDecl = $4.typeDecl;        
    }
    
    | TOKEN_UNSAFE TOKEN_CAST function_cast_cost type_declaration {
        $$.flags = StubFlags(StubFlag::Cast) | StubFlag::Unsafe;
        $$.name = boomer::StringID("Cast");
        $$.location = $1.location;
        $$.intValue = $3.intValue;
        $$.typeDecl = $4.typeDecl;
    }
    ;
    
function_alias
    : TOKEN_ALIAS '(' TOKEN_IDENT ')' { $$.name = $3.name; }
    | /* empty */ { $$.name = boomer::StringID(); }
    ;
    
function_cast_cost
    : '(' TOKEN_INT_NUMBER ')' { $$.intValue = $2.intValue; }
    ;
    
function_operator_type
    : '+' { $$.location = $1.location; $$.name = boomer::StringID("opAdd"); }
    | '-' { $$.location = $1.location; $$.name = boomer::StringID("opSubtract"); }
    | '*' { $$.location = $1.location; $$.name = boomer::StringID("opMultiply"); }
    | '/' { $$.location = $1.location; $$.name = boomer::StringID("opDivide"); }
    | '%' { $$.location = $1.location; $$.name = boomer::StringID("opModulo"); }
    | '$' { $$.location = $1.location; $$.name = boomer::StringID("opDolar"); }
    | '#' { $$.location = $1.location; $$.name = boomer::StringID("opHash"); }
    | '@' { $$.location = $1.location; $$.name = boomer::StringID("opAt"); }
    | '^' { $$.location = $1.location; $$.name = boomer::StringID("opHat"); }
    | '!' { $$.location = $1.location; $$.name = boomer::StringID("opNot"); }
    | '~' { $$.location = $1.location; $$.name = boomer::StringID("opBinaryNot"); }
    | '&' { $$.location = $1.location; $$.name = boomer::StringID("opBinaryAnd"); }
    | '|' { $$.location = $1.location; $$.name = boomer::StringID("opBinaryOr"); }
    | '<' { $$.location = $1.location; $$.name = boomer::StringID("opLess"); }
    | '>' { $$.location = $1.location; $$.name = boomer::StringID("opGreater"); }
    | TOKEN_INC_OP { $$.location = $1.location; $$.name = boomer::StringID("opIncrement"); }
    | TOKEN_DEC_OP { $$.location = $1.location; $$.name = boomer::StringID("opDecrement"); }
    | TOKEN_LEFT_OP { $$.location = $1.location; $$.name = boomer::StringID("opShiftLeft"); }
    | TOKEN_RIGHT_OP { $$.location = $1.location; $$.name = boomer::StringID("opShiftRight"); }
    | TOKEN_RIGHT_RIGHT_OP { $$.location = $1.location; $$.name = boomer::StringID("opShiftRightUnsigned"); }
    | TOKEN_LE_OP { $$.location = $1.location; $$.name = boomer::StringID("opLessEqual"); }
    | TOKEN_GE_OP { $$.location = $1.location; $$.name = boomer::StringID("opGreaterEqual"); }
    | TOKEN_EQ_OP { $$.location = $1.location; $$.name = boomer::StringID("opEqual"); }
    | TOKEN_NE_OP { $$.location = $1.location; $$.name = boomer::StringID("opNotEqual"); }
    | TOKEN_AND_OP { $$.location = $1.location; $$.name = boomer::StringID("opLogicAnd"); }
    | TOKEN_OR_OP { $$.location = $1.location; $$.name = boomer::StringID("opLogicOr"); }
    | TOKEN_MUL_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opMulAssign"); }
    | TOKEN_DIV_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opDivAssign"); }
    | TOKEN_MOD_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opModAssign"); }
    | TOKEN_ADD_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opAddAssign"); }
    | TOKEN_SUB_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opSubAssign"); }
    | TOKEN_LEFT_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opShiftLeftAssign"); }
    | TOKEN_RIGHT_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opShiftRightAssign"); }
    | TOKEN_RIGHT_RIGHT_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opShiftRightUnsignedAssign"); }
    | TOKEN_AT_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opAtAssign"); }
    | TOKEN_HAT_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opHatAssign"); }
    | TOKEN_HASH_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opHashAssign"); }
    | TOKEN_DOLAR_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opDolarAssign"); }
    | TOKEN_AND_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opBinaryAndAssign"); }
    | TOKEN_OR_ASSIGN { $$.location = $1.location; $$.name = boomer::StringID("opBinaryOrAssign"); }
    ;       
   
function_body
    : ';' { $$.tokens.clear(); $$.flags = StubFlags(); $$.name = boomer::StringID(); }     
    | function_code '}' optional_semicolon { $$.flags = StubFlags(); $$.tokens = $1.tokens; $$.name = boomer::StringID(); }
    | TOKEN_OPCODE '(' TOKEN_IDENT ')' ';' { $$.flags = StubFlag::Opcode; $$.name = $3.name; $$.tokens.clear(); }
    ;
    
function_code    
    : '{' { tokens.extractInnerTokenStream('}', $$.tokens); yyclearin; }
    ;    

//---

type_declaration
    : type_declaration '[' TOKEN_INT_NUMBER ']' { $$.typeDecl = context.createStaticArrayType($2.location, $1.typeDecl, $3.intValue); }
    | TOKEN_ARRAY '<' type_declaration '>' { $$.typeDecl = context.createDynamicArrayType($2.location, $3.typeDecl); }
    | type_declaration_simple { $$ = $1; }
    ;

type_declaration_simple
    : type_ref { $$.typeDecl = context.createSimpleType($1.location, $1.typeRef); }
    | TOKEN_CLASS '<' type_ref '>' { $$.typeDecl = context.createClassType($1.location, $3.typeRef); }
    | TOKEN_PTR '<' type_ref '>' { $$.typeDecl = context.createPointerType($1.location, $3.typeRef); }   
    | TOKEN_WEAK '<' type_ref '>' { $$.typeDecl = context.createWeakPointerType($1.location, $3.typeRef); }
    | TOKEN_ENGINE_TYPE TOKEN_NAME { $$.typeDecl = context.createEngineType($1.location, $2.name); }
    ;
    
type_ref
    : type_ref_name { $$.typeRef = context.createTypeRef($1.location, $1.name); }
    ;
    
type_ref_name
    : TOKEN_IDENT
    | type_ref_name '.' TOKEN_IDENT { $$.name = boomer::StringID(boomer::TempString("{}.{}", $1.name, $3.name)); }
    ; 

//---

decl_global_flags
    : decl_flags { context.globalFlags = $1.flags; }
    ;
    
decl_flags
    : decl_flags decl_flag { $$.flags = $1.flags | $2.flags; }
    | /* empty */ { $$.flags = StubFlags(); }
    ;

decl_flag
    : TOKEN_IMPORT { $$.flags = StubFlags(StubFlag::Import) | StubFlags(StubFlag::Native); }
    | TOKEN_ABSTRACT { $$.flags = StubFlag::Abstract; }
    | TOKEN_PROTECTED { $$.flags = StubFlag::Protected; }
    | TOKEN_PRIVATE { $$.flags = StubFlag::Private; }
    | TOKEN_EDITABLE  { $$.flags = StubFlag::Editable; }
    | TOKEN_STATIC { $$.flags = StubFlag::Static; }
    | TOKEN_FINAL { $$.flags = StubFlag::Final; }
    | TOKEN_OVERRIDE { $$.flags = StubFlag::Override; }
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

