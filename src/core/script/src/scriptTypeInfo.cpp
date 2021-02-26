/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptTypeInfo.h"
#include "core/object/include/rttiHandleType.h"
#include "core/object/include/rttiArrayType.h"
#include "core/object/include/rttiClassRefType.h"
#include "core/object/include/rttiProperty.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

TypeInfo::TypeInfo(Type type)
    : nativeType(type)
{}

StringID TypeInfo::name() const
{
    return nativeType ? nativeType->name() : StringID();
}

bool TypeInfo::valid() const
{
    return nativeType != nullptr;
}

bool TypeInfo::scripted() const
{
    return nativeType ? nativeType->scripted() : false;
}

bool TypeInfo::isSimple() const
{
    return nativeType ? (nativeType->metaType() == rtti::MetaType::Simple) : false;
}

bool TypeInfo::isClass() const
{
    return nativeType ? (nativeType->metaType() == rtti::MetaType::Class) : false;
}

bool TypeInfo::isEnum() const
{
    return nativeType ? (nativeType->metaType() == rtti::MetaType::Enum) : false;
}

bool TypeInfo::isPointer() const
{
    if (nativeType && (nativeType->metaType() == rtti::MetaType::WeakHandle || nativeType->metaType() == rtti::MetaType::StrongHandle))
        return true;

    return false;
}

bool TypeInfo::isStrongPointer() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::StrongHandle)
        return true;

    return false;
}

bool TypeInfo::isWeakPointer() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::WeakHandle)
        return true;

    return false;
}

bool TypeInfo::isArray() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Array)
        return true;

    return false;
}

bool TypeInfo::isDynamicArray() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Array)
    {
        auto arrayType  = static_cast<const rtti::IArrayType*>(nativeType.ptr());
        return arrayType->arrayMetaType() == rtti::ArrayMetaType::Dynamic;
    }

    return false;
}

bool TypeInfo::isStaticArray() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Array)
    {
        auto arrayType  = static_cast<const rtti::IArrayType*>(nativeType.ptr());
        return arrayType->arrayMetaType() == rtti::ArrayMetaType::Native;
    }

    return false;
}

ClassTypeInfo TypeInfo::asClass() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Class)
        return ClassTypeInfo(nativeType.toClass());

    return ClassTypeInfo();
}

ArrayTypeInfo TypeInfo::asArray() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Array)
        return ArrayTypeInfo(static_cast<const rtti::IArrayType*>(nativeType.ptr()));

    return ArrayTypeInfo();
}

EnumTypeInfo TypeInfo::asEnum() const
{
    if (nativeType && nativeType->metaType() == rtti::MetaType::Enum)
        return EnumTypeInfo(static_cast<const rtti::EnumType*>(nativeType.ptr()));

    return EnumTypeInfo();
}

ClassTypeInfo TypeInfo::pointedClass() const
{
    return ClassTypeInfo(nativeType.referencedClass());
}

RTTI_BEGIN_TYPE_STRUCT(TypeInfo);
    RTTI_BIND_NATIVE_COMPARE(TypeInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("IsScripted", scripted);
    RTTI_FUNCTION("IsSimple", isSimple);
    RTTI_FUNCTION("IsClass", isClass);
    RTTI_FUNCTION("IsEnum", isEnum);
    RTTI_FUNCTION("IsPointer", isPointer);
    RTTI_FUNCTION("IsStrongPointer", isStrongPointer);
    RTTI_FUNCTION("IsWeakPointer", isWeakPointer);
    RTTI_FUNCTION("IsArray", isArray);
    RTTI_FUNCTION("IsDynamicArray", isDynamicArray);
    RTTI_FUNCTION("IsStaticArray",isStaticArray);
    RTTI_FUNCTION("AsClass", asClass);
    RTTI_FUNCTION("AsArray", asArray);
    RTTI_FUNCTION("AsEnum", asEnum);
    RTTI_FUNCTION("PointedClass", pointedClass);
RTTI_END_TYPE();

//--

ClassTypeInfo::ClassTypeInfo(ClassType type)
    : nativeType(type)
{}

bool ClassTypeInfo::valid() const
{
    return nativeType != nullptr;
}

bool ClassTypeInfo::isAbstract() const
{
    return nativeType && nativeType->isAbstract();
}

bool ClassTypeInfo::scripted() const
{
    return nativeType && nativeType->scripted();
}

bool ClassTypeInfo::isStruct() const
{
    if (nativeType)
        if (nativeType->baseClass() == nullptr)
            return true;

    return false;
}

StringID ClassTypeInfo::name() const
{
    if (nativeType)
        return nativeType->name();
    return StringID();
}

TypeInfo ClassTypeInfo::typeInfo() const
{
    return TypeInfo(nativeType);
}

ClassTypeInfo ClassTypeInfo::baseClassInfo() const
{
    if (nativeType)
        return ClassTypeInfo(nativeType->baseClass());
    return ClassTypeInfo();
}

SpecificClassType<IObject> ClassTypeInfo::classType() const
{
    return nativeType.cast<IObject>();
}

Array<PropertyInfo> ClassTypeInfo::properties() const
{
    if (nativeType != nullptr)
        return (const Array<PropertyInfo>&) nativeType->allProperties();

    static const Array<PropertyInfo> theEmptyList;
    return theEmptyList;
}

Array<PropertyInfo> ClassTypeInfo::localProperties() const
{
    if (nativeType != nullptr)
        return (const Array<PropertyInfo>&) nativeType->localProperties();

    static const Array<PropertyInfo> theEmptyList;
    return theEmptyList;
}

PropertyInfo ClassTypeInfo::findProperty(StringID name) const
{
    if (nativeType != nullptr)
        if (auto prop  = nativeType->findProperty(name))
            return PropertyInfo(prop);

    return PropertyInfo();
}

Array<FunctionInfo> ClassTypeInfo::functions() const
{
    if (nativeType != nullptr)
        return (const Array<FunctionInfo>&) nativeType->allFunctions();

    static const Array<FunctionInfo> theEmptyList;
    return theEmptyList;
}

Array<FunctionInfo> ClassTypeInfo::localFunctions() const
{
    if (nativeType != nullptr)
        return (const Array<FunctionInfo>&) nativeType->localFunctions();

    static const Array<FunctionInfo> theEmptyList;
    return theEmptyList;
}

FunctionInfo ClassTypeInfo::findFunction(StringID name) const
{
    if (nativeType != nullptr)
        if (auto func  = nativeType->findFunction(name))
            return FunctionInfo(func);

    return FunctionInfo();
}

bool ClassTypeInfo::derivesFromClassName(StringID className) const
{
    auto classPtr  = nativeType;
    while (classPtr)
    {
        if (classPtr->name() == className)
            return true;
        classPtr = classPtr->baseClass();
    }

    return false;
}

bool ClassTypeInfo::derivesFromClassInfo(const ClassTypeInfo& classInfo) const
{
    if (nativeType && classInfo.valid())
        return nativeType->is(classInfo.rawType());

    return false;
}

bool ClassTypeInfo::derivesFromClass(SpecificClassType<IObject> classType) const
{
    return nativeType.is(classType);
}

RTTI_BEGIN_TYPE_STRUCT(ClassTypeInfo);
    RTTI_BIND_NATIVE_COMPARE(ClassTypeInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("IsAbstract", isAbstract);
    RTTI_FUNCTION("IsStruct", isStruct);
    RTTI_FUNCTION("IsScripted", scripted);
    RTTI_FUNCTION("GetTypeInfo", typeInfo);
    RTTI_FUNCTION("GetBaseClassInfo", baseClassInfo);
    RTTI_FUNCTION("GetClassType", classType);
    RTTI_FUNCTION("GetProperties", properties);
    RTTI_FUNCTION("GetLocalProperties", localProperties);
    RTTI_FUNCTION("FindProperty", findProperty);
    RTTI_FUNCTION("GetFunctions", functions);
    RTTI_FUNCTION("GetLocalFunctions", localFunctions);
    RTTI_FUNCTION("FindFunction", findFunction);
    RTTI_FUNCTION("DerivesFromClassName", derivesFromClassName);
    RTTI_FUNCTION("DerivesFromClassInfo", derivesFromClassInfo);
    RTTI_FUNCTION("DerivesFromClass", derivesFromClass);
RTTI_END_TYPE();

//---

PropertyInfo::PropertyInfo(const rtti::Property* prop)
    : nativeProperty(prop)
{}

bool PropertyInfo::valid() const
{
    return nativeProperty != nullptr;
}

bool PropertyInfo::scripted() const
{
    return nativeProperty && nativeProperty->scripted();
}

bool PropertyInfo::isEditable() const
{
    return nativeProperty && nativeProperty->editable();
}

bool PropertyInfo::isReadonly() const
{
    return nativeProperty && nativeProperty->readonly();
}

ClassTypeInfo PropertyInfo::parentClassTypeInfo() const
{
    if (nativeProperty)
        return ClassTypeInfo(nativeProperty->parent().toClass());

    return ClassTypeInfo();
}

StringID PropertyInfo::name() const
{
    if (nativeProperty)
        return nativeProperty->name();
    return StringID();
}

StringID PropertyInfo::category() const
{
    if (nativeProperty)
        return nativeProperty->category();
    return StringID();
}

TypeInfo PropertyInfo::typeInfo() const
{
    if (nativeProperty)
        return TypeInfo(nativeProperty->type());
    return TypeInfo();
}

RTTI_BEGIN_TYPE_STRUCT(PropertyInfo);
    RTTI_BIND_NATIVE_COMPARE(PropertyInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("GetCategory", category);
    RTTI_FUNCTION("IsScripted", scripted);
    RTTI_FUNCTION("IsEditable", isEditable);
    RTTI_FUNCTION("IsReadonly", isReadonly);
    RTTI_FUNCTION("GetTypeInfo", typeInfo);
    RTTI_FUNCTION("GetParentClassInfo", parentClassTypeInfo);
RTTI_END_TYPE();

//---

ArrayTypeInfo::ArrayTypeInfo(const rtti::IArrayType* type)
    : nativeType(type)
{}

bool ArrayTypeInfo::valid() const
{
    return nativeType != nullptr;
}

TypeInfo ArrayTypeInfo::typeInfo() const
{
    return TypeInfo(nativeType);
}

TypeInfo ArrayTypeInfo::innerTypeInfo() const
{
    if (nativeType)
        return TypeInfo(nativeType->innerType());
    return TypeInfo();
}

StringID ArrayTypeInfo::name() const
{
    if (nativeType)
        return nativeType->name();
    return StringID();
}

bool ArrayTypeInfo::isStaticSize() const
{
    if (nativeType)
        return nativeType->arrayMetaType() == rtti::ArrayMetaType::Native;
    return false;
}

int ArrayTypeInfo::staticSize() const
{
    if (nativeType && nativeType->arrayMetaType() == rtti::ArrayMetaType::Native)
        return nativeType->arrayCapacity(nullptr);
    return 0;
}

RTTI_BEGIN_TYPE_STRUCT(ArrayTypeInfo);
    RTTI_BIND_NATIVE_COMPARE(ArrayTypeInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("GetTypeInfo", typeInfo);
    RTTI_FUNCTION("GetInnerTypeInfo", innerTypeInfo);
    RTTI_FUNCTION("HasStaticSize", isStaticSize);
    RTTI_FUNCTION("GetStaticSize", staticSize);
RTTI_END_TYPE();

//---

EnumTypeInfo::EnumTypeInfo(const rtti::EnumType* type)
    : nativeType(type)
{}

bool EnumTypeInfo::valid() const
{
    return nativeType != nullptr;
}

bool EnumTypeInfo::scripted() const
{
    return nativeType && nativeType->scripted();
}

StringID EnumTypeInfo::name() const
{
    if (nativeType)
        return nativeType->name();
    return StringID();
}

TypeInfo EnumTypeInfo::typeInfo() const
{
    return TypeInfo(nativeType);
}

int EnumTypeInfo::maxValue() const
{
    return nativeType ? nativeType->maxValue() : 0;
}

int EnumTypeInfo::minValue() const
{
    return nativeType ? nativeType->minValue() : 0;
}

Array<StringID> EnumTypeInfo::options() const
{
    if (nativeType)
        return nativeType->options();

    static Array<StringID> theEmptyArray;
    return theEmptyArray;
}

int EnumTypeInfo::value(StringID optionName, int defaultValue)
{
    int64_t value = defaultValue;
    if (nativeType)
        nativeType->findValue(optionName, value);
    return value;
}

StringID EnumTypeInfo::optionName(int value) const
{
    StringID name;
    if (nativeType)
        nativeType->findName(value, name);
    return name;
}

bool EnumTypeInfo::valueSafe(StringID optionName, int& outValue) const
{
    if (nativeType)
    {
        int64_t value = 0;
        if (nativeType->findValue(optionName, value))
        {
            outValue = value;
            return true;
        }
    }

    return false;
}

int64_t EnumTypeInfo::value64(StringID optionName, int64_t defaultValue)
{
    int64_t value = defaultValue;
    if (nativeType)
        nativeType->findValue(optionName, value);
    return value;
}

StringID EnumTypeInfo::optionName64(int64_t value) const
{
    StringID name;
    if (nativeType)
        nativeType->findName(value, name);
    return name;
}

bool EnumTypeInfo::valueSafe64(StringID optionName, int64_t& outValue) const
{
    if (nativeType)
        return nativeType->findValue(optionName, outValue);

    return false;
}

RTTI_BEGIN_TYPE_STRUCT(EnumTypeInfo);
    RTTI_BIND_NATIVE_COMPARE(EnumTypeInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("IsScripted", scripted);
    RTTI_FUNCTION("GetTypeInfo", typeInfo);
    RTTI_FUNCTION("GetMaxValue", maxValue);
    RTTI_FUNCTION("GetMinValue", minValue);
    RTTI_FUNCTION("GetOptions", options);
    RTTI_FUNCTION("GetValue", value);
    RTTI_FUNCTION("GetValueSafe", valueSafe);
    RTTI_FUNCTION("GetOptionName", optionName);
    RTTI_FUNCTION("GetValue64", value64);
    RTTI_FUNCTION("GetValueSafe64", valueSafe64);
    RTTI_FUNCTION("GetOptionName64", optionName64);
RTTI_END_TYPE();

//---

FunctionInfo::FunctionInfo(const rtti::Function* func)
    : nativeFunction(func)
{}

bool FunctionInfo::valid() const
{
    return nativeFunction != nullptr;
}

bool FunctionInfo::scripted() const
{
    return nativeFunction && nativeFunction->scripted();
}

bool FunctionInfo::isStatic() const
{
    return nativeFunction && nativeFunction->isStatic();
}

bool FunctionInfo::isGlobal() const
{
    return nativeFunction && (nativeFunction->parent() == nullptr);
}

StringID FunctionInfo::name() const
{
    if (nativeFunction)
        return nativeFunction->name();
    return StringID();
}

ClassTypeInfo FunctionInfo::parentClassTypeInfo() const
{
    if (nativeFunction)
        return ClassTypeInfo(Type(nativeFunction->parent()).toClass());
    return ClassTypeInfo();
}

TypeInfo FunctionInfo::returnType() const
{
    if (nativeFunction)
        return TypeInfo(nativeFunction->returnType().m_type);
    return TypeInfo();
}

int FunctionInfo::argumentCount() const
{
    if (nativeFunction)
        return nativeFunction->numParams();
    return 0;
}

TypeInfo FunctionInfo::argumentTypeInfo(int index) const
{
    if (nativeFunction)
    {
        if (index >= 0 && index < (int)nativeFunction->numParams())
            return TypeInfo(nativeFunction->params()[index].m_type);
    }

    return TypeInfo();
}

RTTI_BEGIN_TYPE_STRUCT(FunctionInfo);
    RTTI_BIND_NATIVE_COMPARE(FunctionInfo);
    RTTI_FUNCTION("GetName", name);
    RTTI_FUNCTION("IsValid", valid);
    RTTI_FUNCTION("IsStatic", isStatic);
    RTTI_FUNCTION("IsScripted", scripted);
    RTTI_FUNCTION("IsGlobal", isGlobal);
    RTTI_FUNCTION("GetParentClassInfo", parentClassTypeInfo);
    RTTI_FUNCTION("GetReturnType", returnType);
    RTTI_FUNCTION("GetArgumentCount", argumentCount);
    RTTI_FUNCTION("GetArgumentTypeInfo", argumentTypeInfo);
RTTI_END_TYPE();

//--

static TypeInfo FindTypeInfo(StringID typeName)
{
    auto typeInfo  = RTTI::GetInstance().findType(typeName);
    return TypeInfo(typeInfo);
}

static ClassTypeInfo FindClassInfo(StringID className)
{
    auto classTypeInfo  = RTTI::GetInstance().findClass(className);
    return ClassTypeInfo(classTypeInfo);
}

static ClassTypeInfo GetClassInfo(const SpecificClassType<IObject> classType)
{
    return ClassTypeInfo(classType);
}

static void EnumChildClassInfos(ClassTypeInfo parentClass, Array<ClassTypeInfo>& outClasses)
{
    outClasses.clear();

    InplaceArray<ClassType, 64> derivedClasses;
    RTTI::GetInstance().enumDerivedClasses(parentClass.rawType(), derivedClasses);

    for (auto classPtr  : derivedClasses)
        outClasses.pushBack(ClassTypeInfo(classPtr));
}

static void EnumChildClasses(SpecificClassType<IObject> parentClass, Array<SpecificClassType<IObject>>& outClasses)
{
    outClasses.clear();

    InplaceArray<ClassType, 64> derivedClasses;
    RTTI::GetInstance().enumDerivedClasses(parentClass, derivedClasses);

    for (auto classPtr  : derivedClasses)
        outClasses.pushBack(classPtr.cast<IObject>());
}

static EnumTypeInfo FindEnum(StringID enumName)
{
    auto globalFunction  = RTTI::GetInstance().findEnum(enumName);
    return EnumTypeInfo(globalFunction);
}

static SpecificClassType<IObject> FindClass(StringID className)
{
    return RTTI::GetInstance().findClass(className).cast<IObject>();
}

static FunctionInfo FindFunction(StringID functionName)
{
    auto globalFunction  = RTTI::GetInstance().findGlobalFunction(functionName);
    return FunctionInfo(globalFunction);
}

static ClassTypeInfo GetObjectClassInfo(const ObjectPtr& ptr)
{
    return ptr ? ClassTypeInfo(ptr->cls()) : ClassTypeInfo();
}

static SpecificClassType<IObject> GetObjectClass(const ObjectPtr& ptr)
{
    return ptr ? ptr->cls().cast<IObject>() : nullptr;
}

static StringID GetObjectClassName(const ObjectPtr& ptr)
{
    return ptr ? ptr->cls()->name() : StringID();
}

//--

RTTI_GLOBAL_FUNCTION(FindTypeInfo, "Core.FindTypeInfo");
RTTI_GLOBAL_FUNCTION(FindClassInfo, "Core.FindClassInfo");
RTTI_GLOBAL_FUNCTION(GetClassInfo, "Core.GetClassInfo");
RTTI_GLOBAL_FUNCTION(EnumChildClassInfos, "Core.EnumChildClassInfos");
RTTI_GLOBAL_FUNCTION(EnumChildClasses, "Core.EnumChildClasses");
RTTI_GLOBAL_FUNCTION(FindEnum, "Core.FindEnum");
RTTI_GLOBAL_FUNCTION(FindClass, "Core.FindClass");
RTTI_GLOBAL_FUNCTION(FindFunction, "Core.FindFunction");
RTTI_GLOBAL_FUNCTION(GetObjectClassInfo, "Core.GetObjectClassInfo");
RTTI_GLOBAL_FUNCTION(GetObjectClassName, "Core.GetObjectClassName");
RTTI_GLOBAL_FUNCTION(GetObjectClass, "Core.GetObjectClass");

//--

END_BOOMER_NAMESPACE_EX(script)
