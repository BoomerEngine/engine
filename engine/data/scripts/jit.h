/* JIT compiler for Boomer Script */
/* Automatic file, do not modify */

// common files
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef char char;
typedef short short;
typedef int int;
typedef long int int64_t;
typedef char bool;
typedef float float;
typedef double double;

#define CALL_FINAL 0
#define CALL_VIRTUAL 1

#define INLINE __attribute__((always_inline))

struct FunctionCallingParams
{
    void* _returnPtr;
    void* _argPtr[16];
};

struct ClassPtrWrapper
{
    void* classPtr;
};

struct WeakPtrWrapper
{
    void* holderPtr;
};

struct StrongPtrWrapper
{
    void* dataPtr;
    void* holderPtr;
};

struct ScriptedObject
{
    void* __vtable;
    void* __selfHolder;
    void* __itable;
    void* __parent;
    uint64_t id;
    struct ClassPtrWrapper cls;
    void* __externalProperties;
};

typedef void (*JitFuncPtr)(void* context, void* stackFrame, struct FunctionCallingParams* params);
typedef void (*JitNativeFunctionPtr)();

inline void* ExternalPtr(struct ScriptedObject* ptr)
{
    return ptr->__externalProperties;
}

struct StringIDWrapper
{
    unsigned short _nameId;
};

struct EngineToJIT
{
    void* self;

    void (*_fnLog)(void* self, const char* txt);
    void (*_fnThrowException)(void* self, const void* stackFrame, const char* file, int line, const char* txt);

    void (*_fnTypeCtor)(void* self, int typeId, void* data);
    void (*_fnTypeDtor)(void* self, int typeId, void* data);
    void (*_fnTypeCopy)(void* self, int typeId, void* dest, void* src);
    int (*_fnTypeCompare)(void* self, int typeId, void* a, void* b);

    void (*_fnCall)(void* self, void* context, int funcId, int mode, const void* parentFrame, struct FunctionCallingParams* params);

    void (*_fnNew)(void* self, const void* parentFrame, struct ClassPtrWrapper* cls, struct StrongPtrWrapper* strongPtr);
    bool (*_fnWeakToBool)(void* self, struct WeakPtrWrapper* weakPtr);
    void (*_fnWeakToStrong)(void* self, struct WeakPtrWrapper* weakPtr, struct StrongPtrWrapper* strongPtr);
    void (*_fnStrongToWeak)(void* self, struct StrongPtrWrapper* strongPtr, struct WeakPtrWrapper* weakPtr);
    void (*_fnStrongFromPtr)(void* self, void* ptr, struct StrongPtrWrapper* strongPtr);
    struct StringIDWrapper (*_fnEnumToName)(void* self, int typeId, int64_t enumValue);
    int64_t (*_fnNameToEnum)(void* self, void* context, int typeId, struct StringIDWrapper enumName);
    void (*_fnDynamicStrongCast)(void* self, struct ClassPtrWrapper* classPtr, struct StrongPtrWrapper* inStrongPtr, struct StrongPtrWrapper* outStrongPtr);
    void (*_fnDynamicWeakCast)(void* self, struct ClassPtrWrapper* classPtr, struct StrongPtrWrapper* inWeakPtr, struct StrongPtrWrapper* outWeakPtr);
    void (*_fnMetaCast)(void* self, void* classPtr, struct ClassPtrWrapper* inClassPtr, struct ClassPtrWrapper* outClassPtr);
    struct StringIDWrapper (*_fnClassToName)(void* self, struct ClassPtrWrapper* classPtr);
    void (*_fnClassToString)(void* self, struct ClassPtrWrapper* classPtr, void* outString);
};

struct JITInit
{
    void* self;

    void (*_fnReportImportCounts)(void* self, int maxTypeId, int maxFuncId);
    void (*_fnReportImportType)(void* self, int typeId, const char* name);
    void (*_fnReportImportFunction)(void* self, int funcId, const char* className, const char* funcName, JitNativeFunctionPtr* nativeFunctionPtr);
    void (*_fnReportExportFunction)(void* self, const char* className, const char* funcName, uint64_t codeHash, JitFuncPtr funcPtr);

    void (*_fnInitStringConst)(void* self, void* str, const char* data);
    void (*_fnInitNameConst)(void* self, void* str, const char* data);
    void (*_fnInitTypeConst)(void* self, void* str, const char* data);
};

struct EngineToJIT* EI = 0;

#define LOG(txt) EI->_fnLog(EI->self, txt)
#define ERROR(file, line, txt) EI->_fnThrowException(EI->self, stackFrame, file, line, txt)
#define CTOR(id,p) EI->_fnTypeCtor(EI->self, id, (p))
#define DTOR(id,p) EI->_fnTypeDtor(EI->self, id, (p))
#define COPY(id,dest,src) EI->_fnTypeCopy(EI->self, id, (dest), (src))
#define COMPARE(id,a,b) EI->_fnTypeCompare(EI->self, id, (a), (b))

#define DCL_COUNTS(numTypes, numFunctions) init->_fnReportImportCounts(init->self, numTypes, numFunctions);
#define DCL_TYPE(id, name) init->_fnReportImportType(init->self, id, name);
#define DCL_GLOBAL_FUNC(id, name, nativePtr) init->_fnReportImportFunction(init->self, id, 0, name, (JitNativeFunctionPtr*)nativePtr);
#define DCL_CLASS_FUNC(id, cls, name, nativePtr) init->_fnReportImportFunction(init->self, id, cls, name, (JitNativeFunctionPtr*)nativePtr);
#define DCL_GLOBAL_JIT(name, hash, ptr) init->_fnReportExportFunction(init->self, 0, name, hash, &ptr);
#define DCL_CLASS_JIT(cls, name, hash, ptr) init->_fnReportExportFunction(init->self, cls, name, hash, &ptr);
#define DCL_CONST_STR(data, txt) init->_fnInitStringConst(init->self, data, txt)
#define DCL_CONST_NAME(data, txt) init->_fnInitNameConst(init->self, data, txt)
#define DCL_CONST_TYPE(data, txt) init->_fnInitTypeConst(init->self, data, txt)

void _start()
{
    /* nothing here */
}





