/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "reflectionTypeName.h"
#include "reflectionClassBuilder.h"
#include "reflectionEnumBuilder.h"
#include "reflectionBitFieldBuilder.h"
#include "reflectionFunctionBuilder.h"

#include "core/object/include/rttiClassRef.h"
#include "core/object/include/rttiNativeClassType.h"
#include "core/object/include/rttiCustomType.h"

///------------------------------
/// RTTI DECLARATIONS 101 
///------------------------------

// How to declare a struct:
//
// struct MY_MODULE_API MyStruct
// {
//   RTTI_DECLARE_NONVIRTUAL_CLASS(MyStruct);
//   float x, y, z;
// }
//
// in .cpp file:
// RTTI_BEGIN_STRUCT_TYPE(MyStruct)
//   RTTI_PROPERTY(x);
//   RTTI_PROPERTY(y);
//   RTTI_PROPERTY(z);
// RTTI_END_TYPE()

// How to declare a virtual base class:
//
// class MY_MODULE_API IMyBaseClass
// {
//   RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IMyBaseClass);
// public:
//   virtual void myMethod() = 0;
// }
//
// in .cpp file:
// RTTI_BEGIN_ABSTRACT_CLASS_TYPE(IMyBaseClass)
// RTTI_END_TYPE()

// How to declare a derived class:
//
// class MY_MODULE_API MyDerivedClass : public IMyBaseClass
// {
//   RTTI_DECLARE_VIRTUAL_CLASS(MyDerivedClass, IMyBaseClass);
// public:
//   virtual void myMethod() override;
// }
//
// in .cpp file:
// RTTI_BEGIN_CLASS_TYPE(MyDerivedClass)
// RTTI_END_TYPE()


///------------------------------
/// CLASS DECLARATIONS PARTS - must be inside the class body, usually in .h but can be in .cpp
///------------------------------

// Declare class as non-virtual class that does not have ANYTHING derived from it and it's type is static
// All typical structures are NONVIRTUAL
#define RTTI_DECLARE_NONVIRTUAL_CLASS(className)\
public:\
    using TBaseClass = className;\
    using StaticClassAllocationPool = ClassAllocationPool; \
    static SpecificClassType<className> GetStaticClass();\
    ClassType nativeClass() const { return GetStaticClass(); }\
    ClassType cls() const { return GetStaticClass(); }\
private:\
    friend void CreateType_##className(const char* typeName);\
    friend void InitType_##className(); \
public:

// Declare class as a root class for a virtual class hierarchy, think "IObject"
// All base classes that have classes dervid from them and 
#define RTTI_DECLARE_VIRTUAL_ROOT_CLASS(className)\
public:\
    using TBaseClass = className;\
    using StaticClassAllocationPool = ClassAllocationPool; \
    static SpecificClassType<className> GetStaticClass();\
    virtual ClassType nativeClass() const { return GetStaticClass(); }\
    virtual ClassType cls() const { return nativeClass(); }\
private:\
    friend void CreateType_##className(const char* typeName);\
    friend void InitType_##className();

#define RTTI_DECLARE_VIRTUAL_CLASS(className, baseClass)\
public:\
    using TBaseClass = baseClass;\
    using StaticClassAllocationPool = ClassAllocationPool; \
    static SpecificClassType<className> GetStaticClass();\
    virtual ClassType nativeClass() const { return GetStaticClass(); }\
private:\
    friend void CreateType_##className(const char* typeName);\
    friend void InitType_##className();

///------------------------------
/// DEFINITION PARTS - must be in .cpp file
///------------------------------

// A class implemented in the C++, basic wrapper
// NOTE: you need to bind specific functions to get proper behavior
#define RTTI_BEGIN_TYPE_NATIVE_CLASS(_type)\
    static rtti::NativeClass* theClass##_type = nullptr;\
    SpecificClassType<_type> _type::GetStaticClass() { return *(SpecificClassType<_type>*) &theClass##_type; }\
    void CreateType_##_type(const char* name) {\
        DEBUG_CHECK(!theClass##_type);\
        theClass##_type = new rtti::NativeClass(name, sizeof(_type), alignof(_type), typeid(_type).hash_code(), _type::StaticClassAllocationPool::TAG);\
        RTTI::GetInstance().registerType(theClass##_type);\
    }\
    void InitType_##_type() {\
        typedef _type TType;\
        theClass##_type->baseClass(reflection::ClassID<typename _type::TBaseClass>());\
        reflection::ClassBuilder builder(theClass##_type);\
        _type* zeroObj = (_type*)0;


// Define an abstract class that has a known size and alignment but no way to construct/destruct it's instances
#define RTTI_BEGIN_TYPE_ABSTRACT_CLASS(_type)\
    RTTI_BEGIN_TYPE_NATIVE_CLASS(_type);

// Define a normal C++ "object" class, has constructor/destructor but does not have a value type stuff - no copy/no assignment
#define RTTI_BEGIN_TYPE_CLASS(_type)\
    RTTI_BEGIN_TYPE_NATIVE_CLASS(_type);\
    RTTI_BIND_NATIVE_CTOR_DTOR(_type);

// Define a normal C++ "structure" class, has constructor/destructor and copy constructor/copy assignment but NO automatic comparison
#define RTTI_BEGIN_TYPE_STRUCT(_type)\
    static_assert(!std::is_polymorphic<_type>::value, "Classes with vtables can't be declared as structs");\
    RTTI_BEGIN_TYPE_NATIVE_CLASS(_type);\
    RTTI_BIND_NATIVE_CTOR_DTOR(_type);\
    RTTI_BIND_NATIVE_COPY(_type);

// Define a normal C++ enumeration, exposed options must be added manually via RTTI_ENUM_OPTION
#define RTTI_BEGIN_TYPE_ENUM(_type)\
    static rtti::EnumType* theEnum##_type = nullptr;\
    void CreateType_##_type(const char* name) {\
        theEnum##_type = new rtti::EnumType(StringID(name), (uint32_t)sizeof(_type), typeid(_type).hash_code());\
        RTTI::GetInstance().registerType(theEnum##_type);\
    }\
    void InitType_##_type() {\
        typedef _type TType;\
        reflection::EnumBuilder builder(theEnum##_type);

// Define a normal C++ enumeration, exposed options must be added manually via RTTI_BITFIELD_OPTION
#define RTTI_BEGIN_TYPE_BITFIELD(_type)\
    static rtti::BitfieldType* theBitfield##_type = nullptr;\
    void CreateType_##_type(const char* name) {\
        theBitfield##_type  = new rtti::BitfieldType(StringID(name), (uint32_t)sizeof(_type), typeid(_type).hash_code());\
        RTTI::GetInstance().registerType(theBitfield##_type);\
    }\
    void InitType_##_type() {\
       typedef _type TType;\
       reflection::BitFieldBuilder builder(theBitfield##_type, _type::FLAGS_TYPE);

// Define a custom type of which all parts are provided by user via the "RTTI_BIND" functions
// NOTE: this allows to integrate saving/loading, printing, parsing and property inspection for ANY type
// NOTE: there's no "DECLARE" macro needed so type declaration itself does not have to change (ie. we can add RTTI on top of types we don't fully own, like std::string, QWidget, etc)
#define RTTI_BEGIN_CUSTOM_TYPE(_type)\
    static rtti::CustomType* theCustomType##_type = nullptr;\
    void CreateType_##_type(const char* name) {\
       theCustomType##_type = new rtti::CustomType(name, sizeof(_type), alignof(_type), typeid(_type).hash_code());\
       RTTI::GetInstance().registerType(theCustomType##_type);\
     }\
     void InitType_##_type() {\
       typedef _type TType;\
       reflection::CustomTypeBuilder builder(theCustomType##_type);

///------------------------------
/// NATIVE TYPES BUILDING BLOCKS
///------------------------------

// Bind a native (c++ implemented) constructor and destructor to the type definition
// We will call those functions in order to initialize instanced of that type
// NOTE: ctor and dtor must be public for this to work (WIP so this is not necessary)
#define RTTI_BIND_NATIVE_CTOR_DTOR(_type) builder.type().bindCtorDtor<_type>()

// Bind a native (c++ implemented) copy constructor and copy assignment (we must implement both!)
// We will call those functions in order to create a copy of the data
#define RTTI_BIND_NATIVE_COPY(_type) builder.type().bindCopy<_type>()

// Bind a native (C++ implemented) operator== for use as type data comparions
#define RTTI_BIND_NATIVE_COMPARE(_type) builder.type().bindCompare<_type>()

// Bind writeBinary and readBinary methods from given type as native binary serialization for the type
#define RTTI_BIND_NATIVE_BINARY_SERIALIZATION(_type) builder.type().bindBinarySerialization<_type>()

// Bind writeXML and readXML methods from given type as native XML serialization for the type
#define RTTI_BIND_NATIVE_XML_SERIALIZATION(_type) builder.type().bindXMLSerialization<_type>()

// Bind print and static Parse method from given type as a ToString/FromString interface for the type
#define RTTI_BIND_NATIVE_PRINT_PARSE(_type) builder.type().bindToStringFromString<_type>()

// Bind standalone "print" method from given type as a default ToString interface, no parsing
#define RTTI_BIND_NATIVE_PRINT(_type) builder.type().bindPrint<_type>()

// Bind a native data support for the type (advanced)
#define RTTI_BIND_NATIVE_DATA_VIEW(_type) builder.type().bindDataView<_type>()

// Bind specific static functions as implementation of binary serialization for the type
#define RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(_writeFunc, _readFunc) builder.type().funcReadBinary = _readFunc; builder.type().funcWriteBinary = _writeFunc

// Bind specific static functions as implementation of XML serialization for the type
#define RTTI_BIND_CUSTOM_XML_SERIALIZATION(_writeFunc, _readFunc) builder.type().funcReadXML = _readFunc; builder.type().funcWriteXML = _writeFunc

// Bind a custom print function for the type
#define RTTI_BIND_CUSTOM_PRINT(_printFunc) builder.type().funcPrintToText = _printFunc

// Bind a custom Parse function for the type
#define RTTI_BIND_CUSTOM_PARSE(_parseFunc) builder.type().funcParseFromText = _parseFunc

///------------------------------
/// TYPES BUILDING BLOCKS
///------------------------------

// add type metadata
// NOTE: only one version of metadata can exist in the whole type, e.g.:
// This will not create 2 separate instances of the MySpecialName metadata:
//    RTTI_METADATA(MySpecialName).setName("a");
//    RTTI_METADATA(MySpecialName).setName("b");
// This may be useful to collect stuff:
//    RTTI_METADATA(DependsOnClass).addClass<SomeClass>();
//    RTTI_METADATA(DependsOnClass).addClass<SomeOtherClass>();
#define RTTI_METADATA(_class)\
    builder.addMetadata<_class>()

// add additional name under which the type can be found, can be used when types are moved to different namespaces or renamed
#define RTTI_OLD_NAME(_name)\
    builder.addOldName(_name)

// Get access to advanced type traits definitions
#define RTTI_TYPE_TRAIT()\
    builder.addTrait()

/// ENUMS

// Add an option to the enum type
#define RTTI_ENUM_OPTION(_option)\
    builder.addOption(#_option, TType::_option);

// Add an option to the enum type with optional comment
#define RTTI_ENUM_OPTION_WITH_HINT(_option, _hint)\
    builder.addOption(#_option, TType::_option);

/// BITFIELDS

// Add an option to an bitfield type
#define RTTI_BITFIELD_OPTION(_option)\
    builder.addOption(#_option, TType::ENUM_TYPE::_option);

// And an option to the bitfield type with optional comment
#define RTTI_BITFIELD_OPTION_WITH_HINT(_option, _hint)\
    builder.addOption(#_option, TType::ENUM_TYPE::_option, _hint);

/// CLASSES/STRUCTS

// change current "property category"
#define RTTI_CATEGORY(_category)\
    builder.category(_category)

/// add a property, type and name are extracted automatically
#define RTTI_PROPERTY(_prop)\
    builder.addProperty(#_prop, zeroObj->_prop)

/// add a property but force a fake type on it (size must match though)
#define RTTI_PROPERTY_FORCE_TYPE(_prop, _type)\
    static_assert(sizeof(zeroObj->_prop) == sizeof(_type), "Aliased type must have the same size");\
    builder.addProperty(#_prop, *(_type*)&zeroObj->_prop)

/// add a completely fake property to the class and assume it's at given offset
#define RTTI_PROPERTY_VIRTUAL(_name, _type, _offset)\
    builder.addProperty(_name, reflection::GetTypeObject<_type>(), _offset)

// Define a type's object function (requires pointer to "this" to work)
#define RTTI_FUNCTION(_name, _func)\
    builder.addFunction<TType>(_name).setupProxy(&TType::_func, MakeMethodPtr(TType, _func))

// Define a type's object function (requires pointer to "this" to work)
#define RTTI_FUNCTION_SIMPLE(_func)\
    builder.addFunction<TType>(#_func).setupProxy(&TType::_func, MakeMethodPtr(TType, _func))

// Define a type's static function (does not require a this pointer)
#define RTTI_STATIC_FUNCTION(_name, _func)\
    builder.addStaticFunction(_name).setupProxy(&TType::_func, MakeFunctionPtr(TType::_func))

/// END

// End the type declaration block, must be last function to call
#define RTTI_END_TYPE()\
    builder.submit(); }

///------------------------------
/// GLOBAL DEFINITION PARTS - must be in .cpp file, not related to a single type
///------------------------------

// Define a static global function callable from anywhere
// Many systems can use it but this is mostly for integration with scripting languages
#define RTTI_GLOBAL_FUNCTION(_func, _name)\
    void RegisterGlobalFunc_##_func() {\
        reflection::FunctionBuilder funcBuilder(_name);\
        reflection::FunctionBuilderStatic globalFuncBuilder(funcBuilder);\
        globalFuncBuilder.setupProxy(&_func, MakeFunctionPtr(_func));\
        funcBuilder.submit(nullptr);\
    }

