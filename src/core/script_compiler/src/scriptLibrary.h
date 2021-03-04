/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/script/include/scriptPortableStubs.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

class StubLibrary;
class TypeCastMatrix;

//---

/// library of compilation entities
class StubLibrary : public NoCopy
{
public:
    StubLibrary(LinearAllocator& mem, StringView primaryModuleName);
    ~StubLibrary();

    //--

    // get the primary module
    INLINE StubModule* primaryModule()  { return m_primaryModule; }
    INLINE const StubModule* primaryModule() const { return m_primaryModule; }

    // get type casting matrix
    INLINE const TypeCastMatrix& typeCastingMatrix() const { return *m_typeCastMatrix; }

    //--

    // build named maps of stuff in the library, this detects duplicates
    bool buildNamedMaps(IErrorHandler& err);

    // validate structure of type library
    bool validate(IErrorHandler& err);

    //--

    // find stub in given context
    const Stub* findStubInContext(StringID name, const Stub* context) const;

    // find all aliased functions
    void findAliasedFunctions(StringID name, uint32_t expectedArgumentCount, Array<const StubFunction*>& outAliasedFunctions) const;

    // find or create a direct type declaration to an existing stub
    const StubTypeDecl* resolveSimpleTypeReference(StringID name, const Stub* context);

    //--

    // create file entry
    StubFile* createFile(StubModule* module, const StringBuf& depotPath, const StringBuf& absolutePath);

    // create a module import dependency
    StubModuleImport* createModuleImport(const StubLocation& location, StubFile* file, StringID name);

    // create an enum option
    StubEnumOption* createEnumOption(const StubLocation& location, Stub* owner, StringID name);

    // create an enum
    StubEnum* createEnum(const StubLocation& location, StringID name, Stub* owner);

    // create a class declaration
    StubClass* createClass(const StubLocation& location, StringID name, Stub* owner);

    // create function argument
    StubFunctionArg* createFunctionArg(const StubLocation& location, StringID name, StubFunction* owner);

    // create function declaration
    StubFunction* createFunction(const StubLocation& location, StringID name, Stub* owner);

    // create property
    StubProperty* createProperty(const StubLocation& location, StringID name, Stub* owner);

    // create a native type declaration
    StubTypeName* createTypeAlias(const StubLocation& location, StringID name, const StubTypeDecl* typeDecl, Stub* owner);

    // create a reference to named type
    // NOTE: this will have to be resolved
    StubTypeRef* createTypeRef(const StubLocation& location, StringID name, const Stub* context);

    // create a already resolved type reference to an existing stub
    StubTypeRef* createTypeRef(const Stub* resolvedStub);

    //--

    // create integer constant value
    StubConstantValue* createConstValueInt(const StubLocation& location, int64_t value);

    // create unsigned constant value
    StubConstantValue* createConstValueUint(const StubLocation& location, uint64_t value);

    // create float constant value
    StubConstantValue* createConstValueFloat(const StubLocation& location, double value);

    // create boolean constant vale
    StubConstantValue* createConstValueBool(const StubLocation& location, bool value);

    // create string constant value
    StubConstantValue* createConstValueString(const StubLocation& location, StringView value);

    // create name constant value
    StubConstantValue* createConstValueName(const StubLocation& location, StringID value);

    // create compound constant value
    StubConstantValue* createConstValueCompound(const StubLocation& location, const StubTypeDecl* structTypeRef);

    // create constant
    StubConstant* createConst(const StubLocation& location, StringID name, Stub* owner);

    //--

    // create an engine type reference
    StubTypeDecl* createEngineType(const StubLocation& location, StringID engineTypeName);

    // create a simple type referencing given stub
    StubTypeDecl* createSimpleType(const StubLocation& location, const Stub* stub);

    // create an class type reference (cls<Entity>)
    StubTypeDecl* createClassType(const StubLocation& location, const Stub* classType);

    // create an shared pointer type reference (ptr<Entity>)
    StubTypeDecl* createSharedPointerType(const StubLocation& location, const Stub* classType);

    // create an weak pointer type reference (ptr<Entity>)
    StubTypeDecl* createWeakPointerType(const StubLocation& location, const Stub* classType);

    // create a dynamic array type
    StubTypeDecl* createDynamicArrayType(const StubLocation& location, const StubTypeDecl* innerType);

    // create a static array type
    StubTypeDecl* createStaticArrayType(const StubLocation& location, const StubTypeDecl* innerType, uint32_t arraySize);

    //--

    // check if we can access given stub
    bool canAccess(const Stub* stub, const Stub* fromStub) const;

    //--

    // import content of other module
    const StubModule* importModule(const StubModule* importModule, StringID importName);

    // prune (remove) unused imports for types and functions
    void pruneUnusedImports();

private:
    LinearAllocator& m_mem;

    //--

    SpinLock m_lock;

    Array<StubTypeRef*> m_unresolvedTypes;
    Array<StubTypeDecl*> m_unresolvedDeclarations;

    Array<StubModule*> m_modules;

    StringBuf m_primaryModuleName;
    StubModule* m_primaryModule;

    HashMap<StringID, StubTypeDecl*> m_cacheEngineTypes;
    HashMap<const Stub*, StubTypeRef*> m_cachedTypeRefs;
    HashMap<const StubTypeRef*, StubTypeDecl*> m_cachedTypeSimpleDecls;

    UniquePtr<TypeCastMatrix> m_typeCastMatrix;

    void attachStub(Stub* owner, Stub* stub);

    void formatTypeName(const StubTypeDecl* typeDecl, StringBuilder& f) const;

    bool isUnaryOperator(StringID name) const;
    bool matchTypeSignature(const StubTypeDecl* a, const StubTypeDecl* b) const;
    bool matchTypeSignature(const StubFunctionArg* a, const StubFunctionArg* b) const;
    bool matchFunctionSignature(const StubFunction* a, const StubFunction* b) const;

    StringID formatOperatorName(const StubFunction* func) const;
    StringID formatCastName(const StubFunction* func) const;

    const Stub* findRootStubInContext(StringID name, const Stub* context) const;
    const Stub* findChildStubInContext(StringID name, const Stub* context) const;

    StubTypeRef* createTypeRef_NoLock(const Stub* resolvedStub);

    //--

    bool buildFunction(IErrorHandler& err, StubFunction* ptr);
    bool buildClass(IErrorHandler& err, StubClass* ptr);
    bool linkClasses(IErrorHandler& err);
    bool linkEnums(IErrorHandler& err);
    bool createAutomaticClassFunctions(IErrorHandler& err);
    bool linkFunctions(IErrorHandler& err);
    bool resolveTypeRefs(IErrorHandler& err);
    bool resolveTypeDecls(IErrorHandler& err);
    bool checkProperties(IErrorHandler& err);
    bool buildCastMatrix(IErrorHandler& err);

    bool linkClass(IErrorHandler& err, StubClass* classStub);
    bool linkEnum(IErrorHandler& err, StubEnum* classStub);
    bool linkFunction(IErrorHandler& err, StubFunction* funcStub);
    bool checkClassProperties(IErrorHandler& err, StubClass* classStub);

    void reportError(IErrorHandler& err, const StubLocation& loc, StringView txt);
    void reportError(IErrorHandler& err, const Stub* stub, StringView txt);
    void reportWarning(IErrorHandler& err, const StubLocation& loc, StringView txt);
    void reportWarning(IErrorHandler& err, const Stub* stub, StringView txt);

    Stub* createImportStub(const Stub* sourceStub, HashMap<const Stub*, Stub*>& cache);
    Stub* importStub(const Stub* sourceStub, HashMap<const Stub*, Stub*>& cache);
    void importFileData(const StubFile* source, StubFile* dest, HashMap<const Stub*, Stub*>& cache);
    void importEnumData(const StubEnum* source, StubEnum* dest, HashMap<const Stub*, Stub*>& cache);
    void importEnumOptionData(const StubEnumOption* source, StubEnumOption* dest, HashMap<const Stub*, Stub*>& cache);
    void importClassData(const StubClass* source, StubClass* dest, HashMap<const Stub*, Stub*>& cache);
    void importPropertyData(const StubProperty* source, StubProperty* dest, HashMap<const Stub*, Stub*>& cache);
    void importFunctionData(const StubFunction* source, StubFunction* dest, HashMap<const Stub*, Stub*>& cache);
    void importFunctionArgData(const StubFunctionArg* source, StubFunctionArg* dest, HashMap<const Stub*, Stub*>& cache);
    void importTypeNameData(const StubTypeName* source, StubTypeName* dest, HashMap<const Stub*, Stub*>& cache);
    void importTypeRefData(const StubTypeRef* source, StubTypeRef* dest, HashMap<const Stub*, Stub*>& cache);
    void importTypeDeclData(const StubTypeDecl* source, StubTypeDecl* dest, HashMap<const Stub*, Stub*>& cache);
    void importConstantData(const StubConstant* source, StubConstant* dest, HashMap<const Stub*, Stub*>& cache);
    void importConstantValueData(const StubConstantValue* source, StubConstantValue* dest, HashMap<const Stub*, Stub*>& cache);
    StubLocation importLocation(const StubLocation& location, HashMap<const Stub*, Stub*>& cache);
};

//--

END_BOOMER_NAMESPACE_EX(script)
