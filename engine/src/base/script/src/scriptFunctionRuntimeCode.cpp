/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptPortableStubs.h"
#include "scriptFunctionStackFrame.h"
#include "scriptFunctionRuntimeCode.h"
#include "base/object/include/rttiProperty.h"
#include "base/io/include/ioFileHandleMemory.h"

namespace base
{
    namespace script
    {
        //---

        IFunctionCodeStubResolver::~IFunctionCodeStubResolver()
        {}

        //---

        FunctionCodeBlock::FunctionCodeBlock()
        {}

        FunctionCodeBlock::~FunctionCodeBlock()
        {}

        void FunctionCodeBlock::release()
        {
            MemDelete(this);
        }

        void FunctionCodeBlock::disableAllBreakpoints()
        {

        }

        bool FunctionCodeBlock::checkBreakpoint(uint32_t line) const
        {
            return false;
        }

        bool FunctionCodeBlock::toggleBreakpoint(uint32_t line, bool status)
        {
            return false;
        }

        bool FunctionCodeBlock::findSourceCodeLine(const uint8_t* codePtr, uint32_t& outLine) const
        {
            return false;
        }

        void FunctionCodeBlock::collectActiveBreakpoints(Array<uint32_t>& outLines) const
        {

        }

        //---

        extern const void* GScriptPointers[];

        class PointerMapper : public ISingleton
        {
            DECLARE_SINGLETON(PointerMapper);

        public:
            PointerMapper()
                : m_next(1)
            {}

            uint16_t mapPointer(const void* ptr)
            {
                if (!ptr)
                    return 0;

                auto lock  = CreateLock(m_lock);

                uint16_t index = 0;
                if (m_map.find(ptr, index))
                    return index;

                index = m_next++;
                GScriptPointers[index] = ptr;
                m_map[ptr] = index;
                return index;
            }

        private:
            SpinLock m_lock;
            HashMap<const void*, uint16_t> m_map;
            uint16_t m_next;
        };

        class LocalVariableMapper : public base::NoCopy
        {
        public:
            LocalVariableMapper(IFunctionCodeStubResolver& stubResolver)
                : m_localDataStorageSize(0)
                , m_localDataStorageAlignment(1)
                , m_stubResolver(stubResolver)
            {}

            uint16_t mapLocal(const StubOpcode* localOpcode)
            {
                ASSERT(localOpcode->value.i >= 0);
                auto portableIndex  = (uint16_t)localOpcode->value.u;

                uint16_t machineIndex = 0;
                if (!m_localVarRemapping.find(portableIndex, machineIndex))
                {
                    machineIndex = (uint16_t)m_locals.size();

                    auto& localEntry = m_locals.emplaceBack();
                    ASSERT(localOpcode->value.name);
                    localEntry.name = localOpcode->value.name;
                    localEntry.type = m_stubResolver.resolveType(localOpcode->stub->asTypeDecl());

                    m_localDataStorageAlignment = std::max<uint32_t>(m_localDataStorageAlignment, localEntry.type->alignment());
                    localEntry.offset = base::Align(m_localDataStorageSize, localEntry.type->alignment());
                    m_localDataStorageSize = localEntry.offset + localEntry.type->size();

                    m_localVarRemapping[portableIndex] = machineIndex;

                    TRACE_INFO("Local var '{}', type '{}' placed at offset {}", localEntry.name, localEntry.type->name(), localEntry.offset);
                }

                return machineIndex;
            }

            //--

            uint32_t m_localDataStorageSize;
            uint32_t m_localDataStorageAlignment;
            Array<FunctionLocalVariable> m_locals;

        private:
            base::HashMap<uint16_t, uint16_t> m_localVarRemapping;
            IFunctionCodeStubResolver& m_stubResolver;
        };

        class CodeWriter : public base::NoCopy
        {
        public:
            CodeWriter(io::IWriteFileHandle& writerStream, IFunctionCodeStubResolver& stubResovler)
                : m_stream(writerStream)
                , m_currentOpcode(nullptr)
                , m_stubResovler(stubResovler)
            {}

            INLINE IFunctionCodeStubResolver& resolver()
            {
                return m_stubResovler;
            }

            INLINE void writeOpcode(Opcode newOpcode)
            {
                // move stream to position when opcode for current instruction should be written (this allows us to rewrite the opcode)
                //m_stream.seek(m_opcodeWriteOffset);

                // write opcode ID
                uint16_t val = (uint16_t)newOpcode;
#ifdef PACKED_OPCODE
                if (val >= 0x80)
                {
                    m_stream.writeValue((uint8_t)((val & 0x7F) | 0x80));
                    m_stream.writeValue((uint8_t)(val >> 7));
                }
                else
                {
                    m_stream.writeValue((uint8_t)(val & 0x7F));
                }
#else
               // m_stream.writeValue(val);
#endif
            }

            INLINE void writeOpcodeHeader(const StubOpcode* op)
            {
                // remember where opcode was written
                ASSERT(!m_opcodeOffsets.contains(op));
                //m_opcodeWriteOffset = (uint32_t)m_stream.pos();
                m_opcodeOffsets[op] = m_opcodeWriteOffset;
                m_currentOpcode = op;

                // DO NOT write the label opcode, waste of space
                if (op->op == Opcode::Label)
                    return;

                // track breakpoints
                if (op->op == Opcode::Breakpoint)
                {
                    auto& breakpointInfo = m_breakpoints.emplaceBack();
                    //breakpointInfo.codeOffset = m_stream.pos();
                    breakpointInfo.sourceLine = op->location.line;
                }

                // write the opcode numerical value
                writeOpcode(op->op);
            }

            template< typename T >
            INLINE void writeValue(const T& data)
            {
                auto offset  = m_stream.pos();

                static_assert(!std::is_pointer<T>::value, "Can't write pointer directly");

                uint8_t emptyData[sizeof(T)];
                memzero(&emptyData, sizeof(emptyData));
                //m_stream.write(&emptyData, sizeof(emptyData));

                //*(T*)base::OffsetPtr(m_stream.data(), offset) = data;
            }

            INLINE void writePointer(const void* data)
            {
                auto index  = PointerMapper::GetInstance().mapPointer(data);
                return writeValue(index);
            }

            INLINE void writeJump(const StubOpcode* target)
            {
                ASSERT( target != nullptr );
                ASSERT(target->op == Opcode::Label);
                auto& info = m_jumps.emplaceBack();
                info.source = m_currentOpcode;
                info.target = target;
               // info.offsetOffset = m_stream.pos();
               // m_stream.writeValue((short)0);
            }

            INLINE Array<FunctionCodeBreakpointPlacement>& breakpoints()
            {
                return m_breakpoints;
            }

            bool finalizeJumps()
            {
                for (auto& jump : m_jumps)
                {
                    // find offset for the jump target
                    uint32_t jumpTargetOffset = 0;
                    if (!m_opcodeOffsets.find(jump.target, jumpTargetOffset))
                    {
                        TRACE_ERROR("{}: error: Unresolved jump target", jump.source->location);
                        return false;
                    }

                    // calculate the jump distance
                    int dist = jumpTargetOffset - (int)(jump.offsetOffset + 2);
                    if (dist > std::numeric_limits<short>::max() || dist < std::numeric_limits<short>::min())
                    {
                        TRACE_ERROR("{}: error: Jump target is to far away from source ({} is not encodable in 16 bits)", jump.source->location, dist);
                        return false;
                    }

                    // write the jump offset
                    //m_stream.seek(jump.offsetOffset);
                    //m_stream.writeValue((short)dist);
                }

                // jumps resolved
                return true;
            }

        private:
            io::IWriteFileHandle& m_stream;
            HashMap<const StubOpcode*, uint32_t> m_opcodeOffsets;
            const StubOpcode* m_currentOpcode;
            uint32_t m_opcodeWriteOffset = 0;

            struct Jump
            {
                uint32_t offsetOffset = 0;
                const StubOpcode* target = nullptr;
                const StubOpcode* source = nullptr;
            };

            Array<Jump> m_jumps;

            Array<FunctionCodeBreakpointPlacement> m_breakpoints;

            IFunctionCodeStubResolver& m_stubResovler;
        };

        static void WriteFunctionCall(CodeWriter& w, const StubOpcode* opcode)
        {
            // get the function to call
            auto func  = w.resolver().resolveFunction(opcode->stub->asFunction());

            // write pointer to function we are calling
            w.writePointer(func);
            w.writeValue<uint32_t>(opcode->value.u); // call encoding
        }

        // returntrue if opcode can be filered out, mostly optimizes opcodes
        static bool FilterOpcode(CodeWriter& w, const StubFunction* func, const StubOpcode* opcode)
        {
            switch (opcode->op)
            {
                case Opcode::ContextDtor:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    return !prop->type()->traits().requiresDestructor;
                }

                case Opcode::LocalDtor:
                {
                    auto type  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                    return !type->traits().requiresDestructor;
                }

                case Opcode::ContextCtor:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    return !prop->type()->traits().requiresConstructor || prop->type()->traits().initializedFromZeroMem;
                }

                case Opcode::LocalCtor:
                {
                    auto type  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                    return !type->traits().requiresConstructor;
                }
            }

            return false;
        }

        static void WriteOpcode(CodeWriter& w, LocalVariableMapper& varMapper, IFunctionCodeStubResolver& stubResolver, const StubFunction* func, const StubOpcode* opcode)
        {
            // try to filter some opcodes that may not be needed for runtime
            if (FilterOpcode(w, func, opcode))
                return;

            // write opcode value
            w.writeOpcodeHeader(opcode);

            // write opcode data
            switch (opcode->op)
            {
                // opcodes that do not have any more data
                case Opcode::Nop:
                case Opcode::Null:
                case Opcode::IntOne:
                case Opcode::IntZero:
                case Opcode::BoolTrue:
                case Opcode::BoolFalse:
                case Opcode::Label:
                case Opcode::ThisObject:
                case Opcode::ThisStruct:
                case Opcode::TestEqual1:
                case Opcode::TestEqual2:
                case Opcode::TestEqual4:
                case Opcode::TestEqual8:
                case Opcode::TestNotEqual1:
                case Opcode::TestNotEqual2:
                case Opcode::TestNotEqual4:
                case Opcode::TestNotEqual8:
                case Opcode::TestSignedLess1:
                case Opcode::TestSignedLess2:
                case Opcode::TestSignedLess4:
                case Opcode::TestSignedLess8:
                case Opcode::TestSignedLessEqual1:
                case Opcode::TestSignedLessEqual2:
                case Opcode::TestSignedLessEqual4:
                case Opcode::TestSignedLessEqual8:
                case Opcode::TestSignedGreater1:
                case Opcode::TestSignedGreater2:
                case Opcode::TestSignedGreater4:
                case Opcode::TestSignedGreater8:
                case Opcode::TestSignedGreaterEqual1:
                case Opcode::TestSignedGreaterEqual2:
                case Opcode::TestSignedGreaterEqual4:
                case Opcode::TestSignedGreaterEqual8:
                case Opcode::TestUnsignedLess1:
                case Opcode::TestUnsignedLess2:
                case Opcode::TestUnsignedLess4:
                case Opcode::TestUnsignedLess8:
                case Opcode::TestUnsignedLessEqual1:
                case Opcode::TestUnsignedLessEqual2:
                case Opcode::TestUnsignedLessEqual4:
                case Opcode::TestUnsignedLessEqual8:
                case Opcode::TestUnsignedGreater1:
                case Opcode::TestUnsignedGreater2:
                case Opcode::TestUnsignedGreater4:
                case Opcode::TestUnsignedGreater8:
                case Opcode::TestUnsignedGreaterEqual1:
                case Opcode::TestUnsignedGreaterEqual2:
                case Opcode::TestUnsignedGreaterEqual4:
                case Opcode::TestUnsignedGreaterEqual8:
                case Opcode::TestFloatEqual4:
                case Opcode::TestFloatEqual8:
                case Opcode::TestFloatNotEqual4:
                case Opcode::TestFloatNotEqual8:
                case Opcode::TestFloatLess4:
                case Opcode::TestFloatLess8:
                case Opcode::TestFloatLessEqual4:
                case Opcode::TestFloatLessEqual8:
                case Opcode::TestFloatGreater4:
                case Opcode::TestFloatGreater8:
                case Opcode::TestFloatGreaterEqual4:
                case Opcode::TestFloatGreaterEqual8:
                case Opcode::ExpandSigned8To16:
                case Opcode::ExpandSigned8To32:
                case Opcode::ExpandSigned8To64:
                case Opcode::ExpandSigned16To32:
                case Opcode::ExpandSigned16To64:
                case Opcode::ExpandSigned32To64:
                case Opcode::ExpandUnsigned8To16:
                case Opcode::ExpandUnsigned8To32:
                case Opcode::ExpandUnsigned8To64:
                case Opcode::ExpandUnsigned16To32:
                case Opcode::ExpandUnsigned16To64:
                case Opcode::ExpandUnsigned32To64:
                case Opcode::Contract64To32:
                case Opcode::Contract64To16:
                case Opcode::Contract64To8:
                case Opcode::Contract32To16:
                case Opcode::Contract32To8:
                case Opcode::Contract16To8:
                case Opcode::FloatToInt8:
                case Opcode::FloatToInt16:
                case Opcode::FloatToInt32:
                case Opcode::FloatToInt64:
                case Opcode::FloatToUint8:
                case Opcode::FloatToUint16:
                case Opcode::FloatToUint32:
                case Opcode::FloatToUint64:
                case Opcode::FloatToDouble:
                case Opcode::FloatFromInt8:
                case Opcode::FloatFromInt16:
                case Opcode::FloatFromInt32:
                case Opcode::FloatFromInt64:
                case Opcode::FloatFromUint8:
                case Opcode::FloatFromUint16:
                case Opcode::FloatFromUint32:
                case Opcode::FloatFromUint64:
                case Opcode::FloatFromDouble:
                case Opcode::DoubleToInt8:
                case Opcode::DoubleToInt16:
                case Opcode::DoubleToInt32:
                case Opcode::DoubleToInt64:
                case Opcode::DoubleToUint8:
                case Opcode::DoubleToUint16:
                case Opcode::DoubleToUint32:
                case Opcode::DoubleToUint64:
                case Opcode::DoubleFromInt8:
                case Opcode::DoubleFromInt16:
                case Opcode::DoubleFromInt32:
                case Opcode::DoubleFromInt64:
                case Opcode::DoubleFromUint8:
                case Opcode::DoubleFromUint16:
                case Opcode::DoubleFromUint32:
                case Opcode::DoubleFromUint64:
                case Opcode::NumberToBool8:
                case Opcode::NumberToBool16:
                case Opcode::NumberToBool32:
                case Opcode::NumberToBool64:
                case Opcode::FloatToBool:
                case Opcode::DoubleToBool:
                case Opcode::AddInt8:
                case Opcode::AddInt16:
                case Opcode::AddInt32:
                case Opcode::AddInt64:
                case Opcode::SubInt8:
                case Opcode::SubInt16:
                case Opcode::SubInt32:
                case Opcode::SubInt64:
                case Opcode::MulSigned8:
                case Opcode::MulSigned16:
                case Opcode::MulSigned32:
                case Opcode::MulSigned64:
                case Opcode::MulUnsigned8:
                case Opcode::MulUnsigned16:
                case Opcode::MulUnsigned32:
                case Opcode::MulUnsigned64:
                case Opcode::DivSigned8:
                case Opcode::DivSigned16:
                case Opcode::DivSigned32:
                case Opcode::DivSigned64:
                case Opcode::DivUnsigned8:
                case Opcode::DivUnsigned16:
                case Opcode::DivUnsigned32:
                case Opcode::DivUnsigned64:
                case Opcode::ModSigned8:
                case Opcode::ModSigned16:
                case Opcode::ModSigned32:
                case Opcode::ModSigned64:
                case Opcode::ModUnsigned8:
                case Opcode::ModUnsigned16:
                case Opcode::ModUnsigned32:
                case Opcode::ModUnsigned64:
                case Opcode::NegSigned8:
                case Opcode::NegSigned16:
                case Opcode::NegSigned32:
                case Opcode::NegSigned64:
                case Opcode::NegFloat:
                case Opcode::NegDouble:
                case Opcode::PreIncrement8:
                case Opcode::PreIncrement16:
                case Opcode::PreIncrement32:
                case Opcode::PreIncrement64:
                case Opcode::PreDecrement8:
                case Opcode::PreDecrement16:
                case Opcode::PreDecrement32:
                case Opcode::PreDecrement64:
                case Opcode::PostIncrement8:
                case Opcode::PostIncrement16:
                case Opcode::PostIncrement32:
                case Opcode::PostIncrement64:
                case Opcode::PostDecrement8:
                case Opcode::PostDecrement16:
                case Opcode::PostDecrement32:
                case Opcode::PostDecrement64:
                case Opcode::AddFloat:
                case Opcode::SubFloat:
                case Opcode::MulFloat:
                case Opcode::DivFloat:
                case Opcode::ModFloat:
                case Opcode::AddDouble:
                case Opcode::SubDouble:
                case Opcode::MulDouble:
                case Opcode::DivDouble:
                case Opcode::ModDouble:
                case Opcode::BitAnd8:
                case Opcode::BitAnd16:
                case Opcode::BitAnd32:
                case Opcode::BitAnd64:
                case Opcode::BitOr8:
                case Opcode::BitOr16:
                case Opcode::BitOr32:
                case Opcode::BitOr64:
                case Opcode::BitXor8:
                case Opcode::BitXor16:
                case Opcode::BitXor32:
                case Opcode::BitXor64:
                case Opcode::BitNot8:
                case Opcode::BitNot16:
                case Opcode::BitNot32:
                case Opcode::BitNot64:
                case Opcode::BitShl8:
                case Opcode::BitShl16:
                case Opcode::BitShl32:
                case Opcode::BitShl64:
                case Opcode::BitShr8:
                case Opcode::BitShr16:
                case Opcode::BitShr32:
                case Opcode::BitShr64:
                case Opcode::BitSar8:
                case Opcode::BitSar16:
                case Opcode::BitSar32:
                case Opcode::BitSar64:
                case Opcode::LogicNot:
                case Opcode::LogicXor:
                case Opcode::LoadInt1:
                case Opcode::LoadInt2:
                case Opcode::LoadInt4:
                case Opcode::LoadInt8:
                case Opcode::LoadUint1:
                case Opcode::LoadUint2:
                case Opcode::LoadUint4:
                case Opcode::LoadUint8:
                case Opcode::LoadFloat:
                case Opcode::LoadDouble:
                case Opcode::LoadStrongPtr:
                case Opcode::LoadWeakPtr:
                case Opcode::AssignInt1:
                case Opcode::AssignInt2:
                case Opcode::AssignInt4:
                case Opcode::AssignInt8:
                case Opcode::AssignUint1:
                case Opcode::AssignUint2:
                case Opcode::AssignUint4:
                case Opcode::AssignUint8:
                case Opcode::AssignFloat:
                case Opcode::AssignDouble:
                case Opcode::AddAssignInt8:
                case Opcode::SubAssignInt8:
                case Opcode::BitAndAssign8:
                case Opcode::BitOrAssign8:
                case Opcode::BitXorAssign8:
                case Opcode::BitShlAssign8:
                case Opcode::BitShrAssign8:
                case Opcode::BitSarAssign8:
                case Opcode::AddAssignInt16:
                case Opcode::SubAssignInt16:
                case Opcode::BitAndAssign16:
                case Opcode::BitOrAssign16:
                case Opcode::BitXorAssign16:
                case Opcode::BitShlAssign16:
                case Opcode::BitShrAssign16:
                case Opcode::BitSarAssign16:
                case Opcode::AddAssignInt32:
                case Opcode::SubAssignInt32:
                case Opcode::BitAndAssign32:
                case Opcode::BitOrAssign32:
                case Opcode::BitXorAssign32:
                case Opcode::BitShlAssign32:
                case Opcode::BitShrAssign32:
                case Opcode::BitSarAssign32:
                case Opcode::AddAssignInt64:
                case Opcode::SubAssignInt64:
                case Opcode::BitAndAssign64:
                case Opcode::BitOrAssign64:
                case Opcode::BitXorAssign64:
                case Opcode::BitShlAssign64:
                case Opcode::BitShrAssign64:
                case Opcode::BitSarAssign64:
                case Opcode::MulAssignSignedInt8:
                case Opcode::DivAssignSignedInt8:
                case Opcode::MulAssignSignedInt16:
                case Opcode::DivAssignSignedInt16:
                case Opcode::MulAssignSignedInt32:
                case Opcode::DivAssignSignedInt32:
                case Opcode::MulAssignSignedInt64:
                case Opcode::DivAssignSignedInt64:
                case Opcode::MulAssignUnsignedInt8:
                case Opcode::DivAssignUnsignedInt8:
                case Opcode::MulAssignUnsignedInt16:
                case Opcode::DivAssignUnsignedInt16:
                case Opcode::MulAssignUnsignedInt32:
                case Opcode::DivAssignUnsignedInt32:
                case Opcode::MulAssignUnsignedInt64:
                case Opcode::DivAssignUnsignedInt64:
                case Opcode::AddAssignFloat:
                case Opcode::SubAssignFloat:
                case Opcode::MulAssignFloat:
                case Opcode::DivAssignFloat:
                case Opcode::AddAssignDouble:
                case Opcode::SubAssignDouble:
                case Opcode::MulAssignDouble:
                case Opcode::DivAssignDouble:
                case Opcode::MinSigned8:
                case Opcode::MinSigned16:
                case Opcode::MinSigned32:
                case Opcode::MinSigned64:
                case Opcode::MaxSigned8:
                case Opcode::MaxSigned16:
                case Opcode::MaxSigned32:
                case Opcode::MaxSigned64:
                case Opcode::MinUnsigned8:
                case Opcode::MinUnsigned16:
                case Opcode::MinUnsigned32:
                case Opcode::MinUnsigned64:
                case Opcode::MaxUnsigned8:
                case Opcode::MaxUnsigned16:
                case Opcode::MaxUnsigned32:
                case Opcode::MaxUnsigned64:
                case Opcode::MinFloat:
                case Opcode::MinDouble:
                case Opcode::MaxFloat:
                case Opcode::MaxDouble:
                case Opcode::ClampSigned8:
                case Opcode::ClampSigned16:
                case Opcode::ClampSigned32:
                case Opcode::ClampSigned64:
                case Opcode::ClampUnsigned8:
                case Opcode::ClampUnsigned16:
                case Opcode::ClampUnsigned32:
                case Opcode::ClampUnsigned64:
                case Opcode::ClampFloat:
                case Opcode::ClampDouble:
                case Opcode::Abs8:
                case Opcode::Abs16:
                case Opcode::Abs32:
                case Opcode::Abs64:
                case Opcode::AbsFloat:
                case Opcode::AbsDouble:
                case Opcode::Sign8:
                case Opcode::Sign16:
                case Opcode::Sign32:
                case Opcode::Sign64:
                case Opcode::SignFloat:
                case Opcode::SignDouble:
                case Opcode::NameToBool:
                case Opcode::ClassToBool:
                case Opcode::ClassToName:
                case Opcode::ClassToString:
                case Opcode::ReturnDirect:
                case Opcode::ReturnLoad1:
                case Opcode::ReturnLoad2:
                case Opcode::ReturnLoad4:
                case Opcode::ReturnLoad8:
                case Opcode::Exit:
                    break; // just opcode is enough

                case Opcode::LogicOr:
                case Opcode::LogicAnd:
                    w.writeJump(opcode->target);
                    break;

                case Opcode::Breakpoint:
                    w.writeValue<uint8_t>(0); // enable/disable flag for the opcode
                    break;

                case Opcode::IntConst1:
                    w.writeValue<char>(opcode->value.i);
                    break;

                case Opcode::IntConst2:
                    w.writeValue<short>(opcode->value.i);
                    break;

                case Opcode::IntConst4:
                    w.writeValue<int>(opcode->value.i);
                    break;

                case Opcode::IntConst8:
                    w.writeValue<int64_t>(opcode->value.i);
                    break;

                case Opcode::UintConst1:
                    w.writeValue<uint8_t>(opcode->value.u);
                    break;

                case Opcode::UintConst2:
                    w.writeValue<uint16_t>(opcode->value.u);
                    break;

                case Opcode::UintConst4:
                    w.writeValue<uint32_t>(opcode->value.u);
                    break;

                case Opcode::UintConst8:
                    w.writeValue<uint64_t>(opcode->value.u);
                    break;

                case Opcode::FloatConst:
                    w.writeValue<float>(opcode->value.f);
                    break;

                case Opcode::DoubleConst:
                    w.writeValue<double>(opcode->value.d);
                    break;

                case Opcode::NameConst:
                    w.writeValue<StringID>(opcode->value.name);
                    break;

                case Opcode::StringConst:
                    w.writeValue<StringBuf>(opcode->value.text);
                    break;

                case Opcode::StaticFunc:
                case Opcode::FinalFunc:
                case Opcode::VirtualFunc:
                    WriteFunctionCall(w, opcode);
                    break;

                case Opcode::LoadAny:
                {
                    auto engineType  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                    w.writePointer(engineType.ptr());
                    break;
                }

                case Opcode::AssignAny:
                    break;

                case Opcode::ReturnAny:
                {
                    auto engineType  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                    w.writePointer(engineType.ptr());
                    break;
                }

                case Opcode::ParamVar:
                {
                    w.writeValue((uint8_t)opcode->value.i);
                    break;
                }

                case Opcode::ContextVar:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    ASSERT(prop->offset() <= 0xFFFF);

                    if (prop->externalBuffer())
                        w.writeOpcode(Opcode::ContextExternalVar);

                    w.writeValue(range_cast<uint16_t>(prop->offset()));
                    break;
                }

                case Opcode::ContextCtor:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    ASSERT(prop->offset() <= 0xFFFF);

                    if (prop->externalBuffer())
                        w.writeOpcode(Opcode::ContextExternalCtor);

                    w.writeValue(range_cast<uint16_t>(prop->offset()));
                    w.writePointer(prop->type().ptr());
                    break;
                }

                case Opcode::ContextDtor:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    ASSERT(prop->offset() <= 0xFFFF);

                    if (prop->externalBuffer())
                        w.writeOpcode(Opcode::ContextExternalDtor);

                    w.writeValue(range_cast<uint16_t>(prop->offset()));
                    w.writePointer(prop->type().ptr());
                    break;
                }

                case Opcode::LocalVar:
                {
                    auto machineIndex = varMapper.mapLocal(opcode);
                    auto variableOffset = varMapper.m_locals[machineIndex].offset;
                    w.writeValue((uint16_t)variableOffset);
                    break;
                }

                case Opcode::LocalCtor:
                case Opcode::LocalDtor:
                {
                    auto machineIndex = varMapper.mapLocal(opcode);
                    auto variableOffset = varMapper.m_locals[machineIndex].offset;
                    w.writeValue((uint16_t)variableOffset);
                    w.writePointer(varMapper.m_locals[machineIndex].type.ptr());
                    break;
                }

                case Opcode::Jump:
                case Opcode::JumpIfFalse:
                {
                    ASSERT(opcode->target != nullptr);
                    w.writeJump(opcode->target);
                    break;
                }

                case Opcode::StructMember:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    auto cls  = w.resolver().resolveClass(opcode->stub->owner->asClass());
                    w.writePointer(cls.ptr());
                    w.writePointer(prop);
                    break;
                }

                case Opcode::StructMemberRef:
                {
                    auto prop  = w.resolver().resolveProperty(opcode->stub->asProperty());
                    ASSERT(prop->offset() <= 0xFFFF);
                    ASSERT(!prop->externalBuffer());
                    w.writeValue(range_cast<uint16_t>(prop->offset()));
                    break;
                }

                case Opcode::ContextFromPtr:
                case Opcode::ContextFromRef:
                case Opcode::ContextFromPtrRef:
                {
                    w.writeJump(opcode->target);
                    if (opcode->stub != nullptr)
                    {
                        auto type  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                        w.writePointer(type.ptr());
                    }
                    else
                    {
                        w.writePointer(nullptr);
                    }
                    break;
                }

                case Opcode::ContextFromValue:
                {
                    if (opcode->stub != nullptr)
                    {
                        auto type  = w.resolver().resolveType(opcode->stub->asTypeDecl());
                        w.writePointer(type.ptr());
                    }
                    else
                    {
                        w.writePointer(nullptr);
                    }
                    break;
                }

                case Opcode::Constructor:
                {
                    auto classStub  = opcode->stub->asTypeDecl()->classType();
                    auto engineClass  = w.resolver().resolveClass(classStub);
                    w.writePointer(engineClass.ptr());
                    w.writeValue((uint8_t)opcode->value.u);

                    InplaceArray<const StubProperty*, 10> initializationProps;
                    for (auto prop  : classStub->stubs)
                        if (prop->asProperty())
                            initializationProps.pushBack(prop->asProperty());

                    ASSERT(opcode->value.u <= initializationProps.size());
                    for (uint32_t i=0; i<opcode->value.u; ++i)
                    {
                        auto engineProp  = engineClass->findProperty(initializationProps[i]->name);
                        ASSERT(engineProp);
                        ASSERT(engineProp->offset() <= 0xFFFF);
                        ASSERT(!engineProp->externalBuffer());

                        w.writeValue(range_cast<uint16_t>(engineProp->offset()));
                    }

                    break;
                }

                case Opcode::EnumToInt32:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);

                    auto enumSigned = enumType->minValue() < 0;
                    if (enumType->size() == 1)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned8To32 : Opcode::ExpandUnsigned8To32);
                    else if (enumType->size() == 2)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned16To32 : Opcode::ExpandUnsigned16To32);
                    else if (enumType->size() == 4)
                        w.writeOpcode(Opcode::Passthrough);
                    else if (enumType->size() == 8)
                        w.writeOpcode(Opcode::Contract64To32);
                    break;
                }

                case Opcode::EnumToInt64:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);

                    auto enumSigned = enumType->minValue() < 0;
                    if (enumType->size() == 1)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned8To64 : Opcode::ExpandUnsigned8To64);
                    else if (enumType->size() == 2)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned16To64 : Opcode::ExpandUnsigned16To64);
                    else if (enumType->size() == 4)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned32To64 : Opcode::ExpandUnsigned32To64);
                    else if (enumType->size() == 8)
                        w.writeOpcode(Opcode::Passthrough);
                    break;
                }

                case Opcode::Int32ToEnum:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);

                    auto enumSigned = enumType->minValue() < 0;
                    if (enumType->size() == 1)
                        w.writeOpcode(Opcode::Contract32To8);
                    else if (enumType->size() == 2)
                        w.writeOpcode(Opcode::Contract32To16);
                    else if (enumType->size() == 4)
                        w.writeOpcode(Opcode::Passthrough);
                    else if (enumType->size() == 8)
                        w.writeOpcode(enumSigned ? Opcode::ExpandSigned32To64 : Opcode::ExpandUnsigned32To64);
                    break;
                }

                case Opcode::Int64ToEnum:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);

                    if (enumType->size() == 1)
                        w.writeOpcode(Opcode::Contract64To8);
                    else if (enumType->size() == 2)
                        w.writeOpcode(Opcode::Contract64To16);
                    else if (enumType->size() == 4)
                        w.writeOpcode(Opcode::Contract64To32);
                    else if (enumType->size() == 8)
                        w.writeOpcode(Opcode::Passthrough);
                    break;
                }

                case Opcode::EnumToString:
                case Opcode::EnumToName:
                case Opcode::NameToEnum:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);
                    w.writePointer(enumType);
                    break;
                }

                case Opcode::TestEqual:
                case Opcode::TestNotEqual:
                {
                    auto equal = (opcode->op == Opcode::TestEqual);
                    auto boundType  = w.resolver().resolveType(opcode->stub->asTypeDecl());

                    if (boundType->traits().simpleCopyCompare)
                    {
                        uint8_t trivialSize = boundType->size();
                        if (trivialSize == 1)
                            w.writeOpcode(equal ? Opcode::TestEqual1 : Opcode::TestNotEqual1);
                        else if (trivialSize == 2)
                            w.writeOpcode(equal ? Opcode::TestEqual2 : Opcode::TestNotEqual2);
                        else if (trivialSize == 4)
                            w.writeOpcode(equal ? Opcode::TestEqual4 : Opcode::TestNotEqual4);
                        else if (trivialSize == 8)
                            w.writeOpcode(equal ? Opcode::TestEqual8 : Opcode::TestNotEqual8);
                    }
                    else
                    {
                        w.writePointer(boundType.ptr());

                        uint8_t flags = 3; // TODO
                        w.writeValue(flags);
                    }

                    break;
                }

                case Opcode::EnumConst:
                {
                    auto enumStub  = opcode->stub->asEnum();
                    auto enumType  = w.resolver().resolveEnum(enumStub);
                    auto enumSigned = enumType->minValue() < 0;

                    int64_t valueToWrite = 0;
                    enumType->findValue(opcode->value.name, valueToWrite);

                    if (enumType->size() == 1)
                    {
                        w.writeOpcode( enumSigned ? Opcode::IntConst1 : Opcode::UintConst1);
                        w.writeValue<uint8_t>(valueToWrite);
                    }
                    else if (enumType->size() == 2)
                    {
                        w.writeOpcode( enumSigned ? Opcode::IntConst2 : Opcode::UintConst2);
                        w.writeValue<uint16_t>(valueToWrite);
                    }
                    else if (enumType->size() == 4)
                    {
                        w.writeOpcode( enumSigned ? Opcode::IntConst4 : Opcode::UintConst4);
                        w.writeValue<uint32_t>(valueToWrite);
                    }
                    else
                    {
                        w.writeOpcode( enumSigned ? Opcode::IntConst8 : Opcode::UintConst8);
                        w.writeValue<uint64_t>(valueToWrite);
                    }

                    break;
                }

                case Opcode::New:
                case Opcode::MetaCast:
                case Opcode::DynamicCast:
                case Opcode::ClassConst:
                {
                    auto classType = w.resolver().resolveClass(opcode->stub->asClass());
                    w.writePointer(classType.ptr());
                    break;
                }

                case Opcode::WeakToStrong:
                case Opcode::WeakToBool:
                case Opcode::StrongToWeak:
                case Opcode::StrongToBool:
                    break;

                case Opcode::Switch:
                case Opcode::SwitchLabel:
                case Opcode::SwitchDefault:
                case Opcode::Conditional:
                case Opcode::InternalFunc:
                case Opcode::CastToVariant:
                case Opcode::CastFromVariant:
                case Opcode::VariantIsValid:
                case Opcode::VariantIsPointer:
                case Opcode::VariantIsArray:
                case Opcode::VariantGetType:
                case Opcode::VariantToString:

                default:
                    TRACE_ERROR("{}: error: Unsupported opcode '{}'", opcode->location, opcode->op);
                    FATAL_ERROR("Function translation error");
                    break;
            }
        }

        FunctionCodeBlock* FunctionCodeBlock::Create(const StubFunction* func, const StubClass* funcClass, IFunctionCodeStubResolver& stubResolver)
        {
            auto ret = MemNewPool(POOL_SCRIPTS, FunctionCodeBlock);
            ret->m_name = func->name;
            ret->m_codeHash = func->codeHash;
            ret->m_className = funcClass ? funcClass->name : StringID();
            ret->m_sourceFileName = func->location.file ? func->location.file->absolutePath : "";
            ret->m_sourceFileLine = func->location.line;

            //

            // Call 'VectorAdd', ref:Vector, ref:Vector
            //   Call 'VectorMul' ref:Vector, float -> Vector
            //     Property -> ref:Vector
            //     ConstFloat -> float

            // translate opcodes
            io::MemoryWriterFileHandle streamWriter;// (32768, POOL_SCRIPTS);
            CodeWriter codeWriter(streamWriter, stubResolver);
            LocalVariableMapper varMapper(stubResolver);
            for (auto op  : func->opcodes)
                WriteOpcode(codeWriter, varMapper, stubResolver, func, op);

            // fixup all jumps
            if (!codeWriter.finalizeJumps())
            {
                TRACE_ERROR("{}: error: failed to fixup jumps in the function '{}'", func->location, func->name);
                MemDelete(ret.ptr);
                return nullptr;
            }

            // setup variables
            ret->m_locals = std::move(varMapper.m_locals);
            ret->m_localStorageSize = varMapper.m_localDataStorageSize;
            ret->m_localStorageAlignment = varMapper.m_localDataStorageAlignment;

            // extract code
            ret->m_code = streamWriter.extract();
            ret->m_breakpoints = std::move(codeWriter.breakpoints());

            // we now have a machine dependedt VM code we can execute for this function
            return ret;
        }

        //---

        void FunctionCodeBlock::run(const rtti::IFunctionStackFrame* parent, void* context, const rtti::FunctionCallingParams& params) const
        {
            // allocate storage for function local variables
            void *localStorage = nullptr;
            if (m_localStorageAlignment > 0)
            {
                localStorage = AlignPtr(alloca(m_localStorageSize + m_localStorageAlignment - 1), m_localStorageAlignment);
                memzero(localStorage, m_localStorageSize);
            }

            // create stack frame and run the function
            auto parentFrame = static_cast<const StackFrame *>(parent);
            StackFrame frame(parentFrame, context, this, &params, localStorage);
            frame.run();
        }

        //---

    } // script
} // base