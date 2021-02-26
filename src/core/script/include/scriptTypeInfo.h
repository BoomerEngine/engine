/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//----

struct ClassTypeInfo;
struct ArrayTypeInfo;
struct EnumTypeInfo;
struct PropertyInfo;
struct FunctionInfo;

//----

// generic type information wrapper for scripts
struct CORE_SCRIPT_API TypeInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(TypeInfo)

public:
    TypeInfo(Type type = nullptr);
    INLINE TypeInfo(const TypeInfo& other) = default;
    INLINE TypeInfo& operator=(const TypeInfo& other) = default;

    // get the engine name of the type
    StringID name() const;

    // is this a valid type ?
    bool valid() const;

    // was this type defined in scripts ?
    bool scripted() const;

    // is this a simple value type ? (bool/float, etc)
    bool isSimple() const;

    // is this a structure/class type
    bool isClass() const;

    // is this an enum type
    bool isEnum() const;

    // is this a general pointer type
    bool isPointer() const;

    // is this a shared pointer type
    bool isStrongPointer() const;

    // is this a weak pointer type
    bool isWeakPointer() const;

    // is this a generic array
    bool isArray() const;

    // is this a dynamic array
    bool isDynamicArray() const;

    // is this a static array
    bool isStaticArray() const;

    //--

    // get as class type info
    ClassTypeInfo asClass() const;

    // get as class type info
    ArrayTypeInfo asArray() const;

    // get as enum type info
    EnumTypeInfo asEnum() const;

    // for pointers get the pointer class
    ClassTypeInfo pointedClass() const;

    //--

    INLINE bool operator==(const TypeInfo& other) const { return nativeType == other.nativeType; }
    INLINE bool operator!=(const TypeInfo& other) const { return nativeType != other.nativeType; }

    INLINE Type rawType() const { return nativeType; }

private:
    Type nativeType;
};

//----

// class type information wrapper for scripts
struct CORE_SCRIPT_API ClassTypeInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ClassTypeInfo)

public:
    ClassTypeInfo(ClassType type = nullptr);
    INLINE ClassTypeInfo(const ClassTypeInfo& other) = default;
    INLINE ClassTypeInfo& operator=(const ClassTypeInfo& other) = default;

    //--

    // is this a valid type ?
    bool valid() const;

    // is this abstract class ?
    bool isAbstract() const;

    // is this a structure type ? (not an object type)
    bool isStruct() const;

    // is this a structure type ? (not an object type)
    bool scripted() const;

    // get the engine name of the class
    StringID name() const;

    // get type info for this class
    TypeInfo typeInfo() const;

    // get base class
    ClassTypeInfo baseClassInfo() const;

    //--

    // get class type, valid only for classes deriving from object
    SpecificClassType<IObject> classType() const;

    //--

    // get all properties
    Array<PropertyInfo> properties() const;

    // get local properties of this class only
    Array<PropertyInfo> localProperties() const;

    // find property
    PropertyInfo findProperty(StringID name) const;

    //--

    // get all functions
    Array<FunctionInfo> functions() const;

    // get local functions of this class only
    Array<FunctionInfo> localFunctions() const;

    // find function
    FunctionInfo findFunction(StringID name) const;

    //--

    // check if this object implements given class by name (SLOWEST)
    bool derivesFromClassName(StringID className) const;

    // check if this object implements given class
    bool derivesFromClassInfo(const ClassTypeInfo& classInfo) const;

    // check if this object implements given class
    bool derivesFromClass(SpecificClassType<IObject> classType) const;

    //--

    INLINE bool operator==(const ClassTypeInfo& other) const { return nativeType == other.nativeType; }
    INLINE bool operator!=(const ClassTypeInfo& other) const { return nativeType != other.nativeType; }

    INLINE ClassType rawType() const { return nativeType; }

    //--

private:
    ClassType nativeType;
};

//----

/// wrapper for property
struct CORE_SCRIPT_API PropertyInfo
{
public:
    RTTI_DECLARE_NONVIRTUAL_CLASS(PropertyInfo)

public:
    PropertyInfo(const rtti::Property* prop = nullptr);
    INLINE PropertyInfo(const PropertyInfo& other) = default;
    INLINE PropertyInfo& operator=(const PropertyInfo& other) = default;

    // is this property valid ?
    bool valid() const;

    // is this property scripted ?
    bool scripted() const;

    // is this property editable ?
    bool isEditable() const;

    // is this property readonly ?
    bool isReadonly() const;

    // get the class this property was defined at
    ClassTypeInfo parentClassTypeInfo() const;

    // get name of the property
    StringID name() const;

    // get display category of the property
    StringID category() const;

    // get type of the property
    TypeInfo typeInfo() const;

    //--

    INLINE bool operator==(const PropertyInfo& other) const { return nativeProperty == other.nativeProperty; }
    INLINE bool operator!=(const PropertyInfo& other) const { return nativeProperty != other.nativeProperty; }

    INLINE const rtti::Property* rawProperty() const { return nativeProperty; }

    //--

private:
    const rtti::Property* nativeProperty;
};

//----

// array type information wrapper for scripts
struct CORE_SCRIPT_API ArrayTypeInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ArrayTypeInfo)

public:
    ArrayTypeInfo(const rtti::IArrayType* type = nullptr);
    INLINE ArrayTypeInfo(const ArrayTypeInfo& other) = default;
    INLINE ArrayTypeInfo& operator=(const ArrayTypeInfo& other) = default;

    //--

    // is this a valid type ?
    bool valid() const;

    // get type info for this class
    TypeInfo typeInfo() const;

    // get the inner type of the array
    TypeInfo innerTypeInfo() const;

    // get the engine name of the type
    StringID name() const;

    // is this an array with static size ?
    bool isStaticSize() const;

    // get the static size for the array
    int staticSize() const;

    //--

    INLINE bool operator==(const ArrayTypeInfo& other) const { return nativeType == other.nativeType; }
    INLINE bool operator!=(const ArrayTypeInfo& other) const { return nativeType != other.nativeType; }

    INLINE const rtti::IArrayType* rawType() const { return nativeType; }

    //--

private:
    const rtti::IArrayType* nativeType;
};

//--

/// type wrapper for enums
struct CORE_SCRIPT_API EnumTypeInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(EnumTypeInfo)

public:
    EnumTypeInfo(const rtti::EnumType* type = nullptr);
    INLINE EnumTypeInfo(const EnumTypeInfo& other) = default;
    INLINE EnumTypeInfo& operator=(const EnumTypeInfo& other) = default;

    //--

    // is this enum type valid ?
    bool valid() const;

    // is this enum defined in scripts
    bool scripted() const;

    // get name of the enum
    StringID name() const;

    // get back the generic type info
    TypeInfo typeInfo() const;

    //--

    // get maximum enum value
    int maxValue() const;

    // get minumum enum value
    int minValue() const;

    // get all enum names
    Array<StringID> options() const;

    // find value for given option name, returns default value if not found
    int value(StringID optionName, int defaultValue);

    // get name for given value, returns empty name if not found
    StringID optionName(int value) const;

    // find value for given option name, returns false if not found
    bool valueSafe(StringID optionName, int& outValue) const;

    //--

    // find value for given option name, returns default value if not found
    int64_t value64(StringID optionName, int64_t defaultValue);

    // get name for given value, returns empty name if not found
    StringID optionName64(int64_t value) const;

    // find value for given option name, returns false if not found
    bool valueSafe64(StringID optionName, int64_t& outValue) const;

    //--

    INLINE bool operator==(const EnumTypeInfo& other) const { return nativeType == other.nativeType; }
    INLINE bool operator!=(const EnumTypeInfo& other) const { return nativeType != other.nativeType; }

    INLINE const rtti::EnumType* rawType() const { return nativeType; }

    //--

private:
    const rtti::EnumType* nativeType;
};

//--

/// type wrapper for function
struct CORE_SCRIPT_API FunctionInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(FunctionInfo)

public:
    FunctionInfo(const rtti::Function* func = nullptr);
    INLINE FunctionInfo(const FunctionInfo& other) = default;
    INLINE FunctionInfo& operator=(const FunctionInfo& other) = default;

    //--

    // is this function valid ?
    bool valid() const;

    // is this function defined in scripts
    bool scripted() const;

    // is this function static ?
    bool isStatic() const;

    // is this function global ?
    bool isGlobal() const;

    // get name of the function
    StringID name() const;

    // get the owning class, invalid for global functions
    ClassTypeInfo parentClassTypeInfo() const;

    //--

    // get return type
    TypeInfo returnType() const;

    // get number of arguments
    int argumentCount() const;

    // get type of n-th argument
    TypeInfo argumentTypeInfo(int index) const;

    //--

    INLINE bool operator==(const FunctionInfo& other) const { return nativeFunction == other.nativeFunction; }
    INLINE bool operator!=(const FunctionInfo& other) const { return nativeFunction != other.nativeFunction; }

    INLINE const rtti::Function* rawFunction() const { return nativeFunction; }

    //--

private:
    const rtti::Function* nativeFunction;
};

//--


END_BOOMER_NAMESPACE_EX(script)
