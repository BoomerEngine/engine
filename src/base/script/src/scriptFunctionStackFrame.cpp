/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptOpcodes.h"
#include "scriptObject.h"
#include "scriptFunctionRuntimeCode.h"
#include "scriptFunctionStackFrame.h"

#include "base/object/include/rttiProperty.h"

#define ALLOCA_ALIGNED(x, a) ::base::AlignPtr(alloca((x) + (a) - 1), a);
#define ALLOCA_ALIGNED_TYPE(t) ALLOCA_ALIGNED(t->size(), t->alignment())

//#define RETURN(t, x) { t ret = (x); if (resultPtr) { *(t*)resultPtr = ret; } }
#define RETURN(t, x) *(t*)resultPtr = (x);

typedef void (*TOpcodePtr)(base::script::StackFrame* stack, void* resultPtr) noexcept(true);
#define DECLARE_OPCODE(__opcode) static void op##__opcode(StackFrame* stack, void* resultPtr) noexcept(true)

BEGIN_BOOMER_NAMESPACE(base::script)

//---

static TOpcodePtr GOpcodes[(uint16_t)Opcode::Max];

static const uint32_t MAX_POINTERS = 65536;
const void* GScriptPointers[MAX_POINTERS] = { nullptr };

INLINE Opcode ReadOpcode(StackFrame* stack)
{
#ifdef PACKED_OPCODE
    auto & code = stack->codePtr();
    uint16_t ret = *code++;
    if (ret & 0x80)
    {
        ret &= 0x7F;
        ret |= *code++ << 7;
    }
    return ret;
#else
    auto ret  = *(const uint16_t*) stack->codePtr();
    stack->codePtr() += 2;
    return (Opcode)ret;
#endif
}

INLINE void StepGeneric(StackFrame* stack, void* resultPtr) noexcept
{
    auto op  = ReadOpcode(stack);
    //if (stack->parent() == nullptr)
    //    fprintf(stderr, "opcode %s at %s(%d), %i left\n", base::reflection::GetEnumValueName(op), stack->functionName().c_str(), stack->sourceLine(), stack->codeEndPtr() - stack->codePtr());
    (*GOpcodes[(uint16_t)op])(stack, resultPtr);
}

static uint8_t GStratchPad[1024];

INLINE uint8_t EvalUint8(StackFrame* stack) noexcept
{
    uint8_t ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE uint16_t EvalUint16(StackFrame* stack) noexcept
{
    uint16_t ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE uint32_t EvalUint32(StackFrame* stack) noexcept
{
    uint32_t ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE uint64_t EvalUint64(StackFrame* stack) noexcept
{
    uint64_t ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE char EvalInt8(StackFrame* stack) noexcept
{
    char ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE short EvalInt16(StackFrame* stack) noexcept
{
    short ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE int EvalInt32(StackFrame* stack) noexcept
{
    int ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE int64_t EvalInt64(StackFrame* stack) noexcept
{
    int64_t ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE bool EvalBool(StackFrame* stack) noexcept
{
    bool ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE float EvalFloat(StackFrame* stack) noexcept
{
    float ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

INLINE double EvalDouble(StackFrame* stack) noexcept
{
    double ret = 0;
    StepGeneric(stack, &ret);
    return ret;
}

template< typename T = void >
INLINE T* EvalRef(StackFrame* stack) noexcept
{
    T* ptr = nullptr;
    StepGeneric(stack, &ptr);
    return ptr;
}

INLINE void EvalStatement(StackFrame* stack) noexcept
{
    StepGeneric(stack, GStratchPad);
}

//--

template< typename T >
INLINE static T Read(StackFrame* stack)
{
    auto & code = stack->codePtr();
    auto ret  = (const T*)code;
    code += sizeof(T);
    return *ret;
}

template< typename T >
INLINE static T ReadPointer(StackFrame* stack)
{
    auto index  = Read<uint16_t>(stack);
    auto ret = GScriptPointers[index];
    //fprintf(stderr, "Loaded pointer %p as %d\n", ret, index);
    return (T) ret;
}

//---

#include "scriptFunctionStackFrame_Opcodes.inl"

void InitOpcodeTable()
{
    static bool opcodesInitialized = false;
    if (!opcodesInitialized)
    {
        opcodesInitialized = true;
        memzero(GOpcodes, sizeof(GOpcodes));

        #define OPCODE(x)  GOpcodes[(uint32_t)Opcode::x] = &Opcodes::op##x;
        #include "scriptOpcodesRaw.h"
        #undef OPCODE
    }
}

//---

StackFrame::StackFrame(const StackFrame* parent, void* context, const FunctionCodeBlock* code, const rtti::FunctionCallingParams* params, void* localStorage)
    : m_parent(parent)
    , m_contextObject(context)
    , m_activeContextObject(context)
    , m_codeBlock(code)
    , m_params(params)
    , m_locals((uint8_t*)localStorage)
{
    // get code pointer of the function body, this is the code we will execute
    m_codePtr = code->code();
    m_codeEndPtr = code->codeEnd();
}

StackFrame::~StackFrame()
{
}

StringID StackFrame::functionName() const
{
    return m_codeBlock->name();
}

StringID StackFrame::className() const
{
    return m_codeBlock->className();
}

StringBuf StackFrame::sourceFile() const
{
    return m_codeBlock->sourceFileName();
}

uint32_t StackFrame::sourceLine() const
{
    return m_codeBlock->sourceFileLine();
}

// static const TOpcodeIntPtr GFuncPtr = nullptr;

void StackFrame::run() noexcept
{
    //GFuncPtr = &opIntAddInt32;

    while (m_codePtr < m_codeEndPtr)
        EvalStatement(this);
}

//--

END_BOOMER_NAMESPACE(base::script)
