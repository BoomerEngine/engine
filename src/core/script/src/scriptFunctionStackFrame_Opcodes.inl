/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

namespace Opcodes
{

    DECLARE_OPCODE(Nop)
    {
        // nothing
    }

    DECLARE_OPCODE(Null)
    {
        RETURN(ObjectPtr, ObjectPtr());
    }

    DECLARE_OPCODE(Breakpoint)
    {
        auto flag = Read<uint8_t>(stack);
        StepGeneric(stack, resultPtr);
    }

    DECLARE_OPCODE(NameConst)
    {
        RETURN(StringID, Read<StringID>(stack));
    }

    DECLARE_OPCODE(EnumConst)
    {
        ASSERT(!"Opcode should never be emited to final VM");
    }

    DECLARE_OPCODE(StringConst)
    {
        if (resultPtr)
            *(void **) resultPtr = (void *) stack->codePtr();
        stack->codePtr() += sizeof(StringBuf);
    }

    DECLARE_OPCODE(ClassConst)
    {
        auto classPtr = ReadPointer<const rtti::IClassType*>(stack);
        RETURN(ClassType, classPtr);
    }

    DECLARE_OPCODE(Passthrough)
    {
        StepGeneric(stack, resultPtr);
    }

    DECLARE_OPCODE(Switch)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(SwitchLabel)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(SwitchDefault)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(Label)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(Conditional)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(Constructor)
    {
        // get the type of the struct to construct
        Type type = ReadPointer<const rtti::IType*>(stack);

        // get number of arguments
        auto argCount = Read<uint8_t>(stack);

        // read the offsets
        auto argOffset  = (const uint16_t*)stack->codePtr();
        stack->codePtr() += argCount * sizeof(uint16_t);

        // initialize struct
        if (resultPtr)
        {
            // initialize the memory
            // TODO: make sure we are not passing a prealocated shit here
            //memzero(resultPtr, type->size());
            //type->construct(resultPtr);

            // initialize arguments
            for (uint32_t i=0; i<argCount; ++i)
            {
                auto argData = (char*)resultPtr + argOffset[i];
                StepGeneric(stack, argData);
            }
        }
        else
        {
            // NULL pass - we don't care about the result
            for (uint32_t i=0; i<argCount; ++i)
                StepGeneric(stack, nullptr);
        }
    }

    INLINE static void CallFunction(const rtti::Function* functionToCall, StackFrame* stack, void* resultPtr)
    {
        // prepare calling arguments
        rtti::FunctionCallingParams callingParams;
        callingParams.m_returnPtr = resultPtr;

        // function encoding
        FunctionCallingEncoding encoding;
        auto encodingValue = Read<uint32_t>(stack);
        encoding.value = encodingValue;

        // allocate the temporary memory
        uint8_t* paramBlock = nullptr;
        if (functionToCall->paramsDataBlockSize() > 0)
        {
            paramBlock = (uint8_t*) ALLOCA_ALIGNED(functionToCall->paramsDataBlockSize(), functionToCall->paramsDataBlockAlignment());
            memzero(paramBlock, functionToCall->paramsDataBlockSize());
        }

		// change context in which function arguments are evaluated
		auto prevContext  = stack->activeContext();
		stack->activeContext() = stack->initialContext();

        // prepare parameter storage
        uint32_t argIndex = 0;
        uint32_t destroyParams = 0;
        while (encoding.value != 0)
        {
            // make sure we are no exceeding number of arguments expected by the function
            ASSERT(argIndex < functionToCall->numParams());
            destroyParams <<= 1;

            // decode the param passing convetion
            auto paramEncodingMode = encoding.decode();
            ASSERT(paramEncodingMode != FunctionParamMode::None); // this should never happen

            // prepare storage
            if (paramEncodingMode == FunctionParamMode::Ref)
            {
                // evaluate the reference directly
                StepGeneric(stack, &callingParams.m_argumentsPtr[argIndex]);
            }
            else if (paramEncodingMode == FunctionParamMode::TypedValue || paramEncodingMode == FunctionParamMode::SimpleValue)
            {
                // get type of the value we want to allocate
                auto& paramInfo = functionToCall->params()[argIndex];
                ASSERT(paramInfo.m_offset != 0xFFFF);
                auto paramStorage  = paramBlock + paramInfo.m_offset;

                // evaluate param into the storage
                StepGeneric(stack, paramStorage);
                callingParams.m_argumentsPtr[argIndex] = paramStorage;

                // if this is a typed param remember to destroy it
                if (paramEncodingMode == FunctionParamMode::TypedValue)
                    destroyParams |= 1;
            }

            // go to next argument
            argIndex += 1;
        }

		// restore context
		stack->activeContext() = prevContext;

        // make sure we have exactly the proper argument count
        ASSERT(argIndex == functionToCall->numParams());

        // call the function
        functionToCall->run(stack, stack->activeContext(), callingParams);

        // destroy params that we marked for destruction
        while (0 != destroyParams)
        {
            ASSERT(argIndex > 0);
            argIndex -= 1;

            if (destroyParams & 1)
            {
                auto& paramInfo = functionToCall->params()[argIndex];
                ASSERT(paramInfo.m_offset != 0xFFFF);
                auto paramStorage  = paramBlock + paramInfo.m_offset;
                paramInfo.m_type->destruct(paramStorage);
            }

            destroyParams >>= 1;
        }
    }

    DECLARE_OPCODE(FinalFunc)
    {
        auto functionToCall  = ReadPointer<const rtti::Function*>(stack);
        CallFunction(functionToCall, stack, resultPtr);
    }

    DECLARE_OPCODE(VirtualFunc)
    {
        auto functionToCall  = ReadPointer<const rtti::Function*>(stack);
        CallFunction(functionToCall, stack, resultPtr);
    }

    DECLARE_OPCODE(StaticFunc)
    {
        auto functionToCall  = ReadPointer<const rtti::Function*>(stack);
        CallFunction(functionToCall, stack, resultPtr);
    }

    DECLARE_OPCODE(InternalFunc)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(LoadAny)
    {
        Type type = ReadPointer<const rtti::IType*>(stack);
        void* refPtr = EvalRef(stack);
        type->copy(resultPtr, refPtr);
    }

    DECLARE_OPCODE(LoadStrongPtr)
    {
        auto refPtr  = EvalRef<ObjectPtr>(stack);
        RETURN(ObjectPtr, *refPtr);
    }

    DECLARE_OPCODE(LoadWeakPtr)
    {
        auto refPtr  = EvalRef<ObjectWeakPtr>(stack);
        RETURN(ObjectWeakPtr, *refPtr);
    }

    DECLARE_OPCODE(StructMember)
    {
        // get the type of the struct to construct
        Type type = ReadPointer<const rtti::IType*>(stack);

        // get property to copy out
        auto prop = ReadPointer<const rtti::Property*>(stack);

        if (resultPtr)
        {
            // allocate temporary storage for the struct
            void *storage = ALLOCA_ALIGNED_TYPE(type);
            memzero(storage, type->size());
            type->construct(storage);

            // evaluate the WHOLE structure
            StepGeneric(stack, storage);

            // copy out the property

            auto propStorage = prop->offsetPtr(storage);
            prop->type()->copy(resultPtr, propStorage);

            // destroy the struct
            type->destruct(storage);
        }
        else
        {
            // nothing needed
            StepGeneric(stack, nullptr);
        }
    }

    DECLARE_OPCODE(ContextFromPtr)
    {
        // get skip target in case the context is null
        auto offset = Read<short>(stack);
        auto codePtr  = stack->codePtr() + offset;

        // get the type of return property
        Type type = ReadPointer<const rtti::IType*>(stack);

        // evaluate the context
        RefPtr<IObject> ptr;
        StepGeneric(stack, &ptr);

        // if the pointer is valid execute the inner code, if not jump around it
        if (ptr)
        {
            auto oldContext  = stack->activeContext();
            stack->activeContext() = ptr.get();
            StepGeneric(stack, resultPtr);
            stack->activeContext() = oldContext;
        }
        else
        {
            if (resultPtr && type)
                memzero(resultPtr, type->size());

            stack->throwException("Accessing NULL pointer");
            stack->codePtr() = codePtr;
        }
    }

    DECLARE_OPCODE(ContextFromRef)
    {
        // get skip target in case the context is null
        auto offset = Read<short>(stack);
        auto codePtr  = stack->codePtr() + offset;

        // get the type of return property
        Type type = ReadPointer<const rtti::IType*>(stack);

        // evaluate the context REFERENCE
        void* ptr = EvalRef(stack);
        ASSERT(ptr != nullptr);

        // if the pointer is valid execute the inner code, if not jump around it
        auto oldContext  = stack->activeContext();
        stack->activeContext() = ptr;
        StepGeneric(stack, resultPtr);
        stack->activeContext() = oldContext;
    }

    DECLARE_OPCODE(ContextFromValue)
    {
        // get the type of the struct to construct
        Type type = ReadPointer<const rtti::IType*>(stack);

        // allocate temporary storage for the struct
        void *storage = ALLOCA_ALIGNED_TYPE(type);
        memzero(storage, type->size());
        type->construct(storage);

        // initialize the value
        StepGeneric(stack, storage);

        // run code in context of temp struct
        auto oldContext  = stack->activeContext();
        stack->activeContext() = storage;
        StepGeneric(stack, resultPtr);
        stack->activeContext() = oldContext;

        // destroy the temp struct
        type->destruct(storage);
    }

    DECLARE_OPCODE(ContextFromPtrRef)
    {
        // get skip target in case the context is null
        auto offset = Read<short>(stack);
        auto codePtr  = stack->codePtr() + offset;

        // get the type of return property
        Type type = ReadPointer<const rtti::IType*>(stack);

        // evaluate the context
        ObjectPtr* ptr;
        StepGeneric(stack, &ptr);

        // if the pointer is valid execute the inner code, if not jump around it
        if (ptr && ptr->get())
        {
            auto oldContext  = stack->activeContext();
            stack->activeContext() = ptr->get();
            StepGeneric(stack, resultPtr);
            stack->activeContext() = oldContext;
        }
        else
        {
            if (resultPtr && type)
                memzero(resultPtr, type->size());

            stack->throwException("Accessing NULL pointer");
            stack->codePtr() = codePtr;
        }
    }

    DECLARE_OPCODE(New)
    {
        ClassType targetClass = ReadPointer<const rtti::IClassType*>(stack);

        ClassType runtimeClass;
        StepGeneric(stack, &runtimeClass);

        ObjectPtr ptr;

        if (!runtimeClass)
        {
            stack->throwException(TempString("Trying to create object from null class, expected class '{}", targetClass->name()).c_str());
        }
        else if (!runtimeClass->is(targetClass))
        {
            stack->throwException(TempString("Trying to create object from incompatible class '{}', expected class '{}'", runtimeClass->name().c_str(), targetClass->name()).c_str());
        }
        else if (runtimeClass->isAbstract())
        {
            stack->throwException(TempString("Trying to create object from abstract class '{}', expected class '{}'", runtimeClass->name().c_str(), targetClass->name()).c_str());
        }
        else
        {
            ptr = runtimeClass->create<IObject>();
            if (!ptr)
                stack->throwException(TempString("Creating object from class '{}' failed", runtimeClass->name()).c_str());
        }

        RETURN(ObjectPtr, ptr);
    }

    DECLARE_OPCODE(ThisObject)
    {
        RefPtr<IObject> obj;

        auto objPtr = (IObject*) stack->activeContext();
        if (objPtr)
            obj = AddRef(objPtr);

        RETURN(ObjectPtr, obj);
    }

    DECLARE_OPCODE(EnumToName)
    {
        auto enumType  = ReadPointer<const rtti::EnumType*>(stack);
        auto value = EvalInt64(stack);

        StringID name;
        enumType->toStringID(&value, name);

        RETURN(StringID, name);
    }

    DECLARE_OPCODE(Int32ToEnum)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(Int64ToEnum)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(NameToEnum)
    {
        auto enumType  = ReadPointer<const rtti::EnumType*>(stack);

        StringID name;
        StepGeneric(stack, &name);

        if (resultPtr)
            enumType->fromStringID(name, resultPtr);
    }

    DECLARE_OPCODE(DynamicCast)
    {
        ClassType targetClass  = ReadPointer<const rtti::IClassType*>(stack);

        StepGeneric(stack, resultPtr);

        if (resultPtr)
        {
            auto& ptr = *(ObjectPtr*) resultPtr;

            if (!targetClass || (ptr && !ptr->is(targetClass)))
                ptr.reset();
        }
    }

    DECLARE_OPCODE(DynamicWeakCast)
    {
        ClassType targetClass = ReadPointer<const rtti::IClassType*>(stack);

        StepGeneric(stack, resultPtr);

        if (resultPtr)
        {
            auto& weakPtr = *(ObjectWeakPtr*) resultPtr;
            auto ptr = weakPtr.lock();

            if (!targetClass || (ptr && !ptr->is(targetClass)))
                weakPtr.reset();
        }
    }

    DECLARE_OPCODE(MetaCast)
    {
        ClassType targetClass = ReadPointer<const rtti::IClassType*>(stack);

        StepGeneric(stack, resultPtr);

        if (resultPtr)
        {
            auto& classRef = *(ClassType *) resultPtr;
            if (!classRef.is(targetClass))
                classRef = ClassType();
        }
    }

    DECLARE_OPCODE(WeakToStrong)
    {
        if (resultPtr)
        {
            ObjectWeakPtr weakPtr;
            StepGeneric(stack, &weakPtr);

            auto ptr = weakPtr.lock();
            RETURN(ObjectPtr, ptr);
        }
        else
        {
            StepGeneric(stack, nullptr);
        }
    }

    DECLARE_OPCODE(StrongToWeak)
    {
        if (resultPtr)
        {
            ObjectPtr ptr;
            StepGeneric(stack, &ptr);
            RETURN(ObjectWeakPtr, ptr.weak());
        }
        else
        {
            StepGeneric(stack, nullptr);
        }
    }

    DECLARE_OPCODE(ClassToName)
    {
        ClassType classRef;
        StepGeneric(stack, &classRef);
        RETURN(StringID, classRef.empty() ? StringID() : classRef->name());
    }

    DECLARE_OPCODE(ClassToString)
    {
        ClassType classRef;
        StepGeneric(stack, &classRef);
        RETURN(StringBuf, classRef.empty() ? StringBuf() : StringBuf(classRef->name().view()));
    }

    DECLARE_OPCODE(CastToVariant)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(CastFromVariant)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(VariantIsValid)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(VariantIsPointer)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(VariantIsArray)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(VariantGetType)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(VariantToString)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(EnumToString)
    {
        auto enumType  = ReadPointer<const rtti::EnumType*>(stack);
        auto value = EvalInt64(stack);

        StringID name;
        enumType->toStringID(&value, name);

        RETURN(StringBuf, StringBuf(name.view()));
    }

    DECLARE_OPCODE(DoubleConst)
    {
        RETURN(double, Read<double>(stack));
    }

    DECLARE_OPCODE(LoadDouble)
    {
        RETURN(double, *EvalRef<double>(stack));
    }

    DECLARE_OPCODE(NegDouble)
    {
        auto val = EvalDouble(stack);
        RETURN(double, -val);
    }

    DECLARE_OPCODE(AddDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(double, a + b);
    }

    DECLARE_OPCODE(SubDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(double, a - b);
    }

    DECLARE_OPCODE(MulDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(double, a * b);
    }

    DECLARE_OPCODE(DivDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        if (!b) { stack->throwException("Division by zero"); b = 1; }
        RETURN(double, a / b);
    }

    DECLARE_OPCODE(ModDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        if (!b) { stack->throwException("Division by zero"); b = 1; }
        RETURN(double, fmod(a,b));
    }

    DECLARE_OPCODE(FloatToDouble)
    {
        auto val = EvalFloat(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromInt8)
    {
        auto val = EvalInt8(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromInt16)
    {
        auto val = EvalInt16(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromInt32)
    {
        auto val = EvalInt32(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromInt64)
    {
        auto val = EvalInt64(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromUint8)
    {
        auto val = EvalUint8(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromUint16)
    {
        auto val = EvalUint16(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromUint32)
    {
        auto val = EvalUint32(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(DoubleFromUint64)
    {
        auto val = EvalUint64(stack);
        RETURN(double, val);
    }

    DECLARE_OPCODE(MinDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(double, std::min(a,b));
    }

    DECLARE_OPCODE(MaxDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(double, std::max(a,b));
    }

    DECLARE_OPCODE(ClampDouble)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        auto c = EvalDouble(stack);
        RETURN(double, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(AbsDouble)
    {
        auto val = std::abs(EvalDouble(stack));
        RETURN(double, val);
    }


    DECLARE_OPCODE(AddAssignDouble)
    {
        auto ref  = EvalRef<double>(stack);
        auto b = EvalDouble(stack);
        RETURN(double, *ref += b);
    }

    DECLARE_OPCODE(SubAssignDouble)
    {
        auto ref  = EvalRef<double>(stack);
        auto b = EvalDouble(stack);
        RETURN(double, *ref -= b);
    }

    DECLARE_OPCODE(MulAssignDouble)
    {
        auto ref  = EvalRef<double>(stack);
        auto b = EvalDouble(stack);
        RETURN(double, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignDouble)
    {
        auto ref  = EvalRef<double>(stack);
        auto b = EvalDouble(stack);
        RETURN(double, *ref /= b);
    }

    DECLARE_OPCODE(FloatConst)
    {
        RETURN(float, Read<float>(stack));
    }

    DECLARE_OPCODE(LoadFloat)
    {
		auto refPtr  = EvalRef<float>(stack);
        RETURN(float, *refPtr);
    }

    DECLARE_OPCODE(NegFloat)
    {
        auto val = EvalFloat(stack);
        RETURN(float, -val);
    }

    DECLARE_OPCODE(AddFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(float, a + b);
    }

    DECLARE_OPCODE(SubFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(float, a - b);
    }

    DECLARE_OPCODE(MulFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(float, a * b);
    }

    DECLARE_OPCODE(DivFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        if (!b) { stack->throwException("Division by zero"); b = 1; }
        RETURN(float, a / b);
    }

    DECLARE_OPCODE(ModFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        if (!b) { stack->throwException("Division by zero"); b = 1; }
        RETURN(float, fmod(a,b));
    }

    DECLARE_OPCODE(FloatFromInt8)
    {
        auto val = EvalInt8(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromInt16)
    {
        auto val = EvalInt16(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromInt32)
    {
        auto val = EvalInt32(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromInt64)
    {
        auto val = EvalInt64(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromUint8)
    {
        auto val = EvalUint8(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromUint16)
    {
        auto val = EvalUint16(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromUint32)
    {
        auto val = EvalUint32(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromUint64)
    {
        auto val = EvalUint64(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(FloatFromDouble)
    {
        auto val = EvalDouble(stack);
        RETURN(float, val);
    }

    DECLARE_OPCODE(MinFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(float, std::min(a,b));
    }

    DECLARE_OPCODE(MaxFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(float, std::max(a,b));
    }

    DECLARE_OPCODE(ClampFloat)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        auto c = EvalFloat(stack);
        RETURN(float, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(AbsFloat)
    {
        auto val = std::abs(EvalFloat(stack));
        RETURN(float, val);
    }

    DECLARE_OPCODE(AddAssignFloat)
    {
        auto ref  = EvalRef<float>(stack);
        auto b = EvalFloat(stack);
        RETURN(float, *ref += b);
    }

    DECLARE_OPCODE(SubAssignFloat)
    {
        auto ref  = EvalRef<float>(stack);
        auto b = EvalFloat(stack);
        RETURN(float, *ref -= b);
    }

    DECLARE_OPCODE(MulAssignFloat)
    {
        auto ref  = EvalRef<float>(stack);
        auto b = EvalFloat(stack);
        RETURN(float, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignFloat)
    {
        auto ref  = EvalRef<float>(stack);
        auto b = EvalFloat(stack);
        RETURN(float, *ref /= b);
    }

        DECLARE_OPCODE(IntOne)
    {
        RETURN(int, 1);
    }

    DECLARE_OPCODE(IntZero)
    {
        RETURN(int, 0);
    }

    DECLARE_OPCODE(IntConst1)
    {
        RETURN(char, Read<char>(stack));
    }

    DECLARE_OPCODE(IntConst2)
    {
        RETURN(short, Read<short>(stack));
    }

    DECLARE_OPCODE(IntConst4)
    {
        RETURN(int, Read<int>(stack));
    }

    DECLARE_OPCODE(UintConst1)
    {
        RETURN(uint8_t, Read<uint8_t>(stack));
    }

    DECLARE_OPCODE(UintConst2)
    {
        RETURN(uint16_t, Read<uint16_t>(stack));
    }

    DECLARE_OPCODE(UintConst4)
    {
        RETURN(uint32_t, Read<uint32_t>(stack));
    }

    DECLARE_OPCODE(BoolTrue)
    {
        RETURN(uint8_t, 1);
    }

    DECLARE_OPCODE(BoolFalse)
    {
        RETURN(uint8_t, 0);
    }

    DECLARE_OPCODE(LoadInt1)
    {
        RETURN(char, *EvalRef<char>(stack));
    }

    DECLARE_OPCODE(LoadInt2)
    {
        RETURN(short, *EvalRef<short>(stack));
    }

    DECLARE_OPCODE(LoadInt4)
    {
        RETURN(int, *EvalRef<int>(stack));
    }

    DECLARE_OPCODE(LoadUint1)
    {
        RETURN(uint8_t, *EvalRef<uint8_t>(stack));
    }

    DECLARE_OPCODE(LoadUint2)
    {
        RETURN(uint16_t, *EvalRef<uint16_t>(stack));
    }

    DECLARE_OPCODE(LoadUint4)
    {
        RETURN(uint32_t, *EvalRef<uint32_t>(stack));
    }

    DECLARE_OPCODE(StrongToBool)
    {
        RefPtr<IObject> obj;
        StepGeneric(stack, &obj);
        RETURN(bool, obj.get() != nullptr);
    }

    DECLARE_OPCODE(NameToBool)
    {
        StringID name;
        StepGeneric(stack, &name);
        RETURN(bool, name);
    }

    DECLARE_OPCODE(ClassToBool)
    {
        ClassType classRef;
        StepGeneric(stack, &classRef);
        RETURN(bool, !classRef.empty());
    }

    DECLARE_OPCODE(WeakToBool)
    {
        UntypedRefWeakPtr obj;
        StepGeneric(stack, &obj);
        RETURN(bool, !obj.expired());
    }

    DECLARE_OPCODE(PreIncrement8)
    {
        auto refPtr  = EvalRef<uint8_t>(stack);
        RETURN(uint8_t, ++(*refPtr));
    }

    DECLARE_OPCODE(PreIncrement16)
    {
        auto refPtr  = EvalRef<uint16_t>(stack);
        RETURN(uint16_t, ++(*refPtr));
    }

    DECLARE_OPCODE(PreIncrement32)
    {
        auto refPtr  = EvalRef<uint32_t>(stack);
        RETURN(uint32_t,  ++(*refPtr));
    }

    DECLARE_OPCODE(PreDecrement8)
    {
        auto refPtr  = EvalRef<uint8_t>(stack);
        RETURN(uint8_t,  --(*refPtr));
    }

    DECLARE_OPCODE(PreDecrement16)
    {
        auto refPtr  = EvalRef<uint16_t>(stack);
        RETURN(uint16_t, --(*refPtr));
    }

    DECLARE_OPCODE(PreDecrement32)
    {
        auto refPtr  = EvalRef<uint32_t>(stack);
        RETURN(uint32_t,  --(*refPtr));
    }

    DECLARE_OPCODE(PostIncrement8)
    {
        auto refPtr  = EvalRef<uint8_t>(stack);
        RETURN(uint8_t, (*refPtr)++);
    }

    DECLARE_OPCODE(PostIncrement16)
    {
        auto refPtr  = EvalRef<uint16_t>(stack);
        RETURN(uint16_t,(*refPtr)++);
    }

    DECLARE_OPCODE(PostIncrement32)
    {
        auto refPtr  = EvalRef<uint32_t>(stack);
        RETURN(uint32_t, (*refPtr)++);
    }

    DECLARE_OPCODE(PostDecrement8)
    {
        auto refPtr  = EvalRef<uint8_t>(stack);
        RETURN(uint8_t,  (*refPtr)--);
    }

    DECLARE_OPCODE(PostDecrement16)
    {
        auto refPtr  = EvalRef<uint16_t>(stack);
        RETURN(uint16_t, (*refPtr)--);
    }

    DECLARE_OPCODE(PostDecrement32)
    {
        auto refPtr  = EvalRef<uint32_t>(stack);
        RETURN(uint32_t, (*refPtr)--);
    }

    DECLARE_OPCODE(LogicAnd)
    {
        auto skipOffset = Read<uint16_t>(stack);
        auto endPtr  = stack->codePtr() + skipOffset;

        auto a = EvalBool(stack);
        if (a)
        {
            auto inner = EvalBool(stack);
            RETURN(bool, inner);
        }
        else
        {
            stack->codePtr() = endPtr;
            RETURN(bool, false);
        }
    }

    DECLARE_OPCODE(LogicOr)
    {
        auto skipOffset = Read<uint16_t>(stack);
        auto endPtr  = stack->codePtr() + skipOffset;

        auto a = EvalBool(stack);
        if (!a)
        {
            auto inner = EvalBool(stack);
            RETURN(bool, inner);
        }
        else
        {
            stack->codePtr() = endPtr;
            RETURN(bool, true);
        }
    }

    DECLARE_OPCODE(LogicNot)
    {
        auto inner = EvalBool(stack);
        RETURN(bool, !inner);
    }

    DECLARE_OPCODE(LogicXor)
    {
        auto a = EvalBool(stack);
        auto b = EvalBool(stack);
        RETURN(bool, a ^ b);
    }

    DECLARE_OPCODE(TestEqual)
    {
        Type typePtr  = ReadPointer<const rtti::IType*>(stack);
        auto flags = Read<uint8_t>(stack);

        // evaluate first arg
        void* firstArgData = nullptr;
        if (flags & 1)
        {
            firstArgData = ALLOCA_ALIGNED_TYPE(typePtr);
            memset(firstArgData, 0, typePtr->size());
            typePtr->construct(firstArgData);
            StepGeneric(stack, firstArgData);
        }
        else
        {
            StepGeneric(stack, &firstArgData);
        }

        // evaluate second arg
        void* secondArgData = nullptr;
        if (flags & 2)
        {
            secondArgData = ALLOCA_ALIGNED_TYPE(typePtr);
            memset(secondArgData, 0, typePtr->size());
            typePtr->construct(secondArgData);
            StepGeneric(stack, secondArgData);
        }
        else
        {
            StepGeneric(stack, &secondArgData);
        }

        // compare values
        auto equal = typePtr->compare(firstArgData, secondArgData);
        RETURN(bool, equal);

        // destroy
        if (flags & 1)
            typePtr->destruct(firstArgData);
        if (flags & 2)
            typePtr->destruct(secondArgData);
    }

    DECLARE_OPCODE(TestNotEqual)
    {
        Type typePtr = ReadPointer<const rtti::IType*>(stack);
        auto flags = Read<uint8_t>(stack);

        // evaluate first arg
        void* firstArgData = nullptr;
        if (flags & 1)
        {
            firstArgData = ALLOCA_ALIGNED_TYPE(typePtr);
            memset(firstArgData, 0, typePtr->size());
            typePtr->construct(firstArgData);
            StepGeneric(stack, firstArgData);
        }
        else
        {
            StepGeneric(stack, &firstArgData);
        }

        // evaluate second arg
        void* secondArgData = nullptr;
        if (flags & 2)
        {
            secondArgData = ALLOCA_ALIGNED_TYPE(typePtr);
            memset(secondArgData, 0, typePtr->size());
            typePtr->construct(secondArgData);
            StepGeneric(stack, secondArgData);
        }
        else
        {
            StepGeneric(stack, &secondArgData);
        }

        // compare values
        auto equal = !typePtr->compare(firstArgData, secondArgData);
        RETURN(bool, equal);

        // destroy
        if (flags & 1)
            typePtr->destruct(firstArgData);
        if (flags & 2)
            typePtr->destruct(secondArgData);
    }

    DECLARE_OPCODE(TestEqual1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestEqual2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestEqual4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestEqual8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestNotEqual1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestNotEqual2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestNotEqual4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestNotEqual8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestSignedLess1)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestSignedLess2)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestSignedLess4)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestSignedLess8)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestSignedLessEqual1)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestSignedLessEqual2)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestSignedLessEqual4)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestSignedLessEqual8)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestSignedGreater1)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestSignedGreater2)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestSignedGreater4)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestSignedGreater8)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestSignedGreaterEqual1)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestSignedGreaterEqual2)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestSignedGreaterEqual4)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestSignedGreaterEqual8)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestUnsignedLess1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestUnsignedLess2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestUnsignedLess4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestUnsignedLess8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestUnsignedLessEqual1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestUnsignedLessEqual2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestUnsignedLessEqual4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestUnsignedLessEqual8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestUnsignedGreater1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestUnsignedGreater2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestUnsignedGreater4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestUnsignedGreater8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestUnsignedGreaterEqual1)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestUnsignedGreaterEqual2)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestUnsignedGreaterEqual4)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestUnsignedGreaterEqual8)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestFloatEqual4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestFloatEqual8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a == b);
    }

    DECLARE_OPCODE(TestFloatNotEqual4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestFloatNotEqual8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a != b);
    }

    DECLARE_OPCODE(TestFloatLess4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestFloatLess8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a < b);
    }

    DECLARE_OPCODE(TestFloatLessEqual4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestFloatLessEqual8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a <= b);
    }

    DECLARE_OPCODE(TestFloatGreater4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestFloatGreater8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a > b);
    }

    DECLARE_OPCODE(TestFloatGreaterEqual4)
    {
        auto a = EvalFloat(stack);
        auto b = EvalFloat(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(TestFloatGreaterEqual8)
    {
        auto a = EvalDouble(stack);
        auto b = EvalDouble(stack);
        RETURN(bool, a >= b);
    }

    DECLARE_OPCODE(ExpandSigned8To16)
    {
        auto val = EvalInt8(stack);
        RETURN(short, val);
    }

    DECLARE_OPCODE(ExpandSigned8To32)
    {
        auto val = EvalInt8(stack);
        RETURN(int, val);
    }

    DECLARE_OPCODE(ExpandSigned16To32)
    {
        auto val = EvalInt16(stack);
        RETURN(int, val);
    }

    DECLARE_OPCODE(ExpandUnsigned8To16)
    {
        auto val = EvalUint8(stack);
        RETURN(uint16_t, val);
    }

    DECLARE_OPCODE(ExpandUnsigned8To32)
    {
        auto val = EvalUint8(stack);
        RETURN(uint32_t, val);
    }

    DECLARE_OPCODE(ExpandUnsigned16To32)
    {
        auto val = EvalUint16(stack);
        RETURN(uint32_t, val);
    }

    DECLARE_OPCODE(Contract64To32)
    {
        auto val = EvalUint64(stack);
        RETURN(uint32_t, val);
    }

    DECLARE_OPCODE(Contract64To16)
    {
        auto val = EvalUint64(stack);
        RETURN(uint16_t, val);
    }

    DECLARE_OPCODE(Contract64To8)
    {
        auto val = EvalUint64(stack);
        RETURN(uint8_t, val);
    }

    DECLARE_OPCODE(Contract32To16)
    {
        auto val = EvalUint32(stack);
        RETURN(uint16_t, val);
    }

    DECLARE_OPCODE(Contract32To8)
    {
        auto val = EvalUint32(stack);
        RETURN(uint8_t, val);
    }

    DECLARE_OPCODE(Contract16To8)
    {
        auto val = EvalUint16(stack);
        RETURN(uint8_t, val);
    }

    DECLARE_OPCODE(FloatToInt8)
    {
        auto val = EvalFloat(stack);
        RETURN(char, val);
    }

    DECLARE_OPCODE(FloatToInt16)
    {
        auto val = EvalFloat(stack);
        RETURN(short, val);
    }

    DECLARE_OPCODE(FloatToInt32)
    {
        auto val = EvalFloat(stack);
        RETURN(int, val);
    }

    DECLARE_OPCODE(FloatToUint8)
    {
        auto val = EvalFloat(stack);
        RETURN(uint8_t, val);
    }

    DECLARE_OPCODE(FloatToUint16)
    {
        auto val = EvalFloat(stack);
        RETURN(uint16_t, val);
    }

    DECLARE_OPCODE(FloatToUint32)
    {
        auto val = EvalFloat(stack);
        RETURN(uint32_t, val);
    }


    DECLARE_OPCODE(DoubleToInt8)
    {
        auto val = EvalDouble(stack);
        RETURN(char, val);
    }

    DECLARE_OPCODE(DoubleToInt16)
    {
        auto val = EvalDouble(stack);
        RETURN(short, val);
    }

    DECLARE_OPCODE(DoubleToInt32)
    {
        auto val = EvalDouble(stack);
        RETURN(int, val);
    }

    DECLARE_OPCODE(DoubleToUint8)
    {
        auto val = EvalDouble(stack);
        RETURN(uint8_t, val);
    }

    DECLARE_OPCODE(DoubleToUint16)
    {
        auto val = EvalDouble(stack);
        RETURN(uint16_t, val);
    }

    DECLARE_OPCODE(DoubleToUint32)
    {
        auto val = EvalDouble(stack);
        RETURN(uint32_t, val);
    }

    DECLARE_OPCODE(NumberToBool8)
    {
        auto val = EvalUint8(stack) != 0;
        RETURN(bool, val);
    }

    DECLARE_OPCODE(NumberToBool16)
    {
        auto val = EvalUint16(stack) != 0;
        RETURN(bool, val);
    }

    DECLARE_OPCODE(NumberToBool32)
    {
        auto val = EvalUint32(stack) != 0;
        RETURN(bool, val);
    }

    DECLARE_OPCODE(NumberToBool64)
    {
        auto val = EvalUint64(stack) != 0;
        RETURN(bool, val);
    }

    DECLARE_OPCODE(FloatToBool)
    {
        auto val = EvalFloat(stack) != 0.0f;
        RETURN(bool, val);
    }

    DECLARE_OPCODE(DoubleToBool)
    {
        auto val = EvalFloat(stack) != 0.0;
        RETURN(bool, val);
    }

    //---

    DECLARE_OPCODE(MinUnsigned8)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(uint8_t, std::min(a,b));
    }

    DECLARE_OPCODE(MinUnsigned16)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(uint16_t, std::min(a,b));
    }

    DECLARE_OPCODE(MinUnsigned32)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(uint32_t, std::min(a,b));
    }

    DECLARE_OPCODE(MinSigned8)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(char, std::min(a,b));
    }

    DECLARE_OPCODE(MinSigned16)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(short,  std::min(a,b));
    }

    DECLARE_OPCODE(MinSigned32)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(int, std::min(a,b));
    }

    DECLARE_OPCODE(MaxSigned8)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(char, std::max(a,b));
    }

    DECLARE_OPCODE(MaxSigned16)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(short, std::max(a,b));
    }

    DECLARE_OPCODE(MaxSigned32)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(int, std::max(a,b));
    }

    DECLARE_OPCODE(MaxUnsigned8)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        RETURN(uint8_t, std::max(a,b));
    }

    DECLARE_OPCODE(MaxUnsigned16)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        RETURN(uint16_t, std::max(a,b));
    }

    DECLARE_OPCODE(MaxUnsigned32)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        RETURN(uint32_t, std::max(a,b));
    }

    DECLARE_OPCODE(ClampSigned8)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        auto c = EvalInt8(stack);
        RETURN(char, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampSigned16)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        auto c = EvalInt16(stack);
        RETURN(short, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampSigned32)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        auto c = EvalInt32(stack);
        RETURN(int, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampUnsigned8)
    {
        auto a = EvalUint8(stack);
        auto b = EvalUint8(stack);
        auto c = EvalUint8(stack);
        RETURN(uint8_t, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampUnsigned16)
    {
        auto a = EvalUint16(stack);
        auto b = EvalUint16(stack);
        auto c = EvalUint16(stack);
        RETURN(uint16_t, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampUnsigned32)
    {
        auto a = EvalUint32(stack);
        auto b = EvalUint32(stack);
        auto c = EvalUint32(stack);
        RETURN(uint32_t, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(Abs8)
    {
        auto val = std::abs(EvalInt8(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(Abs16)
    {
        auto val = std::abs(EvalInt16(stack));
        RETURN(short, val);
    }

    DECLARE_OPCODE(Abs32)
    {
        auto val = std::abs(EvalInt32(stack));
        RETURN(int, val);
    }

    template< typename T >
    INLINE static char Sign(T v)
    {
        return (v < 0) ? -1 : (v > 0 ? 1 : 0);
    }

    DECLARE_OPCODE(Sign8)
    {
        auto val = Sign(EvalInt8(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(Sign16)
    {
        auto val = Sign(EvalInt16(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(Sign32)
    {
        auto val = Sign(EvalInt32(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(Sign64)
    {
        auto val = Sign(EvalInt64(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(SignFloat)
    {
        auto val = Sign(EvalFloat(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(SignDouble)
    {
        auto val = Sign(EvalDouble(stack));
        RETURN(char, val);
    }

    DECLARE_OPCODE(AddInt8)
    {
        auto a = EvalInt8(stack);
        auto b = EvalInt8(stack);
        RETURN(char, a + b);
    }

    DECLARE_OPCODE(AddInt16)
    {
        auto a = EvalInt16(stack);
        auto b = EvalInt16(stack);
        RETURN(short, a + b);
    }

    DECLARE_OPCODE(AddInt32)
    {
        auto a = EvalInt32(stack);
        auto b = EvalInt32(stack);
        RETURN(int, a + b);
    }

    DECLARE_OPCODE(SubInt8)
    {
		auto a = EvalInt8(stack);
		auto b = EvalInt8(stack);
		RETURN(char, a + b);
    }

    DECLARE_OPCODE(SubInt16)
    {
		auto a = EvalInt16(stack);
		auto b = EvalInt16(stack);
		RETURN(short, a + b);
    }

    DECLARE_OPCODE(SubInt32)
    {
		auto a = EvalInt32(stack);
		auto b = EvalInt32(stack);
		RETURN(int, a - b);
    }

    DECLARE_OPCODE(MulSigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulSigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulSigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulUnsigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulUnsigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulUnsigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivSigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivSigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivSigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivUnsigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivUnsigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(DivUnsigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModSigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModSigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModSigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModUnsigned8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModUnsigned16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(ModUnsigned32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(NegSigned8)
    {
        auto val = EvalInt8(stack);
        RETURN(char, -val);
    }

    DECLARE_OPCODE(NegSigned16)
    {
        auto val = EvalInt16(stack);
        RETURN(short, -val);
    }

    DECLARE_OPCODE(NegSigned32)
    {
        auto val = EvalInt32(stack);
        RETURN(int, -val);
    }

    DECLARE_OPCODE(BitAnd8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitAnd16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitAnd32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOr8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOr16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOr32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXor8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXor16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXor32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitNot8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitNot16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitNot32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShl8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShl16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShl32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShr8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShr16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShr32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSar8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSar16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSar32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(AddAssignInt8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(SubAssignInt8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitAndAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOrAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXorAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShlAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShrAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSarAssign8)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(AddAssignInt16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(SubAssignInt16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitAndAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOrAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXorAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShlAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShrAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSarAssign16)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(AddAssignInt32)
    {
		auto ref  = EvalRef<int>(stack);
		auto val = EvalInt32(stack);
		*ref += val;
		RETURN(int, *ref);
    }

    DECLARE_OPCODE(SubAssignInt32)
    {
		auto ref  = EvalRef<int>(stack);
		auto val = EvalInt32(stack);
		*ref -= val;
		RETURN(int, *ref);
    }

    DECLARE_OPCODE(BitAndAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitOrAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitXorAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShlAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitShrAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(BitSarAssign32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

    DECLARE_OPCODE(MulAssignSignedInt8)
    {
        auto ref  = EvalRef<char>(stack);
        auto b = EvalInt8(stack);
        RETURN(char, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignSignedInt8)
    {
        auto ref  = EvalRef<char>(stack);
        auto b = EvalInt8(stack);
        RETURN(char, *ref /= b);
    }

    DECLARE_OPCODE(MulAssignSignedInt16)
    {
        auto ref  = EvalRef<short>(stack);
        auto b = EvalInt16(stack);
        RETURN(short, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignSignedInt16)
    {
        auto ref  = EvalRef<short>(stack);
        auto b = EvalInt16(stack);
        RETURN(short, *ref /= b);
    }

    DECLARE_OPCODE(MulAssignSignedInt32)
    {
        auto ref  = EvalRef<int>(stack);
        auto b = EvalInt32(stack);
        RETURN(int, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignSignedInt32)
    {
        auto ref  = EvalRef<int>(stack);
        auto b = EvalInt32(stack);
        RETURN(int, *ref /= b);
    }


    DECLARE_OPCODE(MulAssignUnsignedInt8)
    {
        auto ref  = EvalRef<uint8_t>(stack);
        auto b = EvalUint8(stack);
        RETURN(uint8_t, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignUnsignedInt8)
    {
        auto ref  = EvalRef<uint8_t>(stack);
        auto b = EvalUint8(stack);
        RETURN(uint8_t, *ref /= b);
    }

    DECLARE_OPCODE(MulAssignUnsignedInt16)
    {
        auto ref  = EvalRef<uint16_t>(stack);
        auto b = EvalUint16(stack);
        RETURN(uint16_t, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignUnsignedInt16)
    {
        auto ref  = EvalRef<uint16_t>(stack);
        auto b = EvalUint16(stack);
        RETURN(uint16_t, *ref /= b);
    }

    DECLARE_OPCODE(MulAssignUnsignedInt32)
    {
        auto ref  = EvalRef<uint32_t>(stack);
        auto b = EvalUint32(stack);
        RETURN(uint32_t, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignUnsignedInt32)
    {
        auto ref  = EvalRef<uint32_t>(stack);
        auto b = EvalUint32(stack);
        RETURN(uint32_t, *ref /= b);
    }

    DECLARE_OPCODE(EnumToInt32)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
        RETURN(int, 0);
    }

    DECLARE_OPCODE(AssignAny)
    {
        auto refPtr = EvalRef(stack);
        StepGeneric(stack, refPtr);
    }

    DECLARE_OPCODE(AssignInt1)
    {
        auto refPtr = EvalRef<char>(stack);
        *refPtr = EvalInt8(stack);
    }

    DECLARE_OPCODE(AssignInt2)
    {
        auto refPtr = EvalRef<short>(stack);
        *refPtr = EvalInt16(stack);
    }

    DECLARE_OPCODE(AssignInt4)
    {
        auto refPtr = EvalRef<int>(stack);
        *refPtr = EvalInt32(stack);
    }

    DECLARE_OPCODE(AssignInt8)
    {
        auto refPtr = EvalRef<int64_t>(stack);
        *refPtr = EvalInt64(stack);
    }

    DECLARE_OPCODE(AssignUint1)
    {
        auto refPtr = EvalRef<uint8_t>(stack);
        *refPtr = EvalUint8(stack);
    }

    DECLARE_OPCODE(AssignUint2)
    {
        auto refPtr = EvalRef<uint8_t>(stack);
        *refPtr = EvalUint16(stack);
    }

    DECLARE_OPCODE(AssignUint4)
    {
        auto refPtr = EvalRef<uint8_t>(stack);
        *refPtr = EvalUint32(stack);
    }

    DECLARE_OPCODE(AssignUint8)
    {
        auto refPtr = EvalRef<uint8_t>(stack);
        *refPtr = EvalUint64(stack);
    }

    DECLARE_OPCODE(AssignFloat)
    {
        auto refPtr = EvalRef<float>(stack);
        *refPtr = EvalFloat(stack);
    }

    DECLARE_OPCODE(AssignDouble)
    {
        auto refPtr = EvalRef<double>(stack);
        *refPtr = EvalFloat(stack);
    }

    DECLARE_OPCODE(Jump)
    {
        auto offset = Read<short>(stack);
        stack->codePtr() += offset;
    }

    DECLARE_OPCODE(JumpIfFalse)
    {
        auto offset = Read<short>(stack);
        auto targetCodePtr = stack->codePtr() + offset;
        auto cond = EvalBool(stack);
        if (!cond) stack->codePtr() = targetCodePtr;
    }

    DECLARE_OPCODE(LocalCtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);
        type->construct(stack->locals() + offset);
        //TRACE_INFO("Local ctor '{}' at offset {}", type->name(), offset);
    }

    DECLARE_OPCODE(LocalDtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);
        //TRACE_INFO("Local Dtor '{}' at offset {}", type->name(), offset);
        type->destruct(stack->locals() + offset);
    }

    DECLARE_OPCODE(ContextCtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);
        type->construct((char*)stack->activeContext() + offset);
        //TRACE_INFO("Context ctor '{}' at offset {}", type->name(), offset);
    }

    DECLARE_OPCODE(ContextDtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);
        //TRACE_INFO("Context Dtor '{}' at offset {}", type->name(), offset);
        type->destruct((char*)stack->activeContext() + offset);
    }

    DECLARE_OPCODE(ContextExternalCtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);
        auto obj  = (ScriptedObject*)stack->activeContext();
        type->construct((char*)obj->scriptedData() + offset);
       // TRACE_INFO("Context external ctor '{}' at offset {}", type->name(), offset);
    }

    DECLARE_OPCODE(ContextExternalDtor)
    {
        auto offset = Read<uint16_t>(stack);
        Type type = ReadPointer<const rtti::IType*>(stack);;
        //TRACE_INFO("Context external Dtor '{}' at offset {}", type->name(), offset);
        auto obj  = (ScriptedObject*)stack->activeContext();
        type->destruct((char*)obj->scriptedData() + offset);
    }
    
        DECLARE_OPCODE(IntConst8)
    {
        RETURN(int64_t, Read<int64_t>(stack));
    }

    DECLARE_OPCODE(UintConst8)
    {
        RETURN(uint64_t, Read<uint64_t>(stack));
    }

    DECLARE_OPCODE(LocalVar)
    {
        auto offset = Read<uint16_t>(stack);
        RETURN(void*, stack->locals() + offset);
    }

    DECLARE_OPCODE(ParamVar)
    {
        auto index = Read<uint8_t>(stack);
        RETURN(void*, stack->params()->m_argumentsPtr[index]);
    }

    DECLARE_OPCODE(ContextVar)
    {
        auto offset = Read<uint16_t>(stack);
        void *context = stack->activeContext();
        RETURN(void*, OffsetPtr(context, offset));
    }

    DECLARE_OPCODE(ContextExternalVar)
    {
        auto offset = Read<uint16_t>(stack);
        auto context  = (script::ScriptedObject*)stack->activeContext();
        auto data  = context->scriptedData();
        RETURN(void*, OffsetPtr(data, offset));
    }

    DECLARE_OPCODE(LoadInt8)
    {
        RETURN(int64_t, *EvalRef<int64_t>(stack));
    }

    DECLARE_OPCODE(LoadUint8)
    {
        RETURN(uint64_t, *EvalRef<uint64_t>(stack));
    }

    DECLARE_OPCODE(StructMemberRef)
    {
        auto offset = Read<uint16_t>(stack);
        void* refPtr = EvalRef<void>(stack);
        RETURN(void*, OffsetPtr(refPtr, offset));
    }

    DECLARE_OPCODE(ThisStruct)
    {
        RETURN(void*, stack->activeContext());
    }

    DECLARE_OPCODE(PreIncrement64)
    {
        auto refPtr  = EvalRef<uint64_t>(stack);
        RETURN(uint64_t,  ++(*refPtr));
    }

    DECLARE_OPCODE(PreDecrement64)
    {
        auto refPtr  = EvalRef<uint64_t>(stack);
        RETURN(uint64_t, --(*refPtr));
    }

    DECLARE_OPCODE(PostIncrement64)
    {
        auto refPtr  = EvalRef<uint64_t>(stack);
        RETURN(uint64_t, (*refPtr)++);
    }

    DECLARE_OPCODE(PostDecrement64)
    {
        auto refPtr  = EvalRef<uint64_t>(stack);
        RETURN(uint64_t, (*refPtr)--);
    }

    DECLARE_OPCODE(ExpandSigned8To64)
    {
        auto val = EvalInt8(stack);
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(ExpandSigned16To64)
    {
        auto val = EvalInt16(stack);
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(ExpandSigned32To64)
    {
        auto val = EvalInt32(stack);
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(ExpandUnsigned8To64)
    {
        auto val = EvalUint8(stack);
        RETURN(uint64_t, val);
    }

    DECLARE_OPCODE(ExpandUnsigned16To64)
    {
        auto val = EvalUint16(stack);
        RETURN(uint64_t, val);
    }

    DECLARE_OPCODE(ExpandUnsigned32To64)
    {
        auto val = EvalUint32(stack);
        RETURN(uint64_t, val);
    }

    DECLARE_OPCODE(FloatToInt64)
    {
        auto val = EvalFloat(stack);
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(FloatToUint64)
    {
        auto val = EvalFloat(stack);
        RETURN(uint64_t, val);
    }

    DECLARE_OPCODE(DoubleToInt64)
    {
        auto val =  EvalDouble(stack);
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(DoubleToUint64)
    {
        auto val = EvalDouble(stack);
        RETURN(uint64_t, val);
    }

    DECLARE_OPCODE(MinUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, std::min(a,b));
    }

    DECLARE_OPCODE(MinSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t,  std::min(a,b));
    }

    DECLARE_OPCODE(MaxSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, std::max(a,b));
    }

    DECLARE_OPCODE(MaxUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, std::max(a,b));
    }

    DECLARE_OPCODE(ClampSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        auto c = EvalInt64(stack);
        RETURN(int64_t, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(ClampUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        auto c = EvalUint64(stack);
        RETURN(uint64_t, std::clamp(a,b,c));
    }

    DECLARE_OPCODE(Abs64)
    {
        auto val = std::abs(EvalInt64(stack));
        RETURN(int64_t, val);
    }

    DECLARE_OPCODE(AddAssignInt64)
    {
        auto ref  = EvalRef<int64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, *ref += b);
    }

    DECLARE_OPCODE(SubAssignInt64)
    {
        auto ref  = EvalRef<int64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, *ref -= b);
    }

    DECLARE_OPCODE(BitAndAssign64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(uint64_t, *ref &= b);
    }

    DECLARE_OPCODE(BitOrAssign64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(uint64_t, *ref |= b);
    }

    DECLARE_OPCODE(BitXorAssign64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(uint64_t, *ref ^= b);
    }

    DECLARE_OPCODE(BitShlAssign64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalUint8(stack);
        RETURN(uint64_t, *ref <<= b);
    }

    DECLARE_OPCODE(BitShrAssign64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalUint8(stack);
        RETURN(uint64_t, *ref >>= b);
    }

    DECLARE_OPCODE(BitSarAssign64)
    {
        auto ref  = EvalRef<int64_t>(stack);
        auto b = EvalUint8(stack);
        RETURN(int64_t, *ref >>= b);
    }

    DECLARE_OPCODE(MulAssignSignedInt64)
    {
        auto ref  = EvalRef<int64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignSignedInt64)
    {
        auto ref  = EvalRef<int64_t>(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, *ref /= b);
    }

    DECLARE_OPCODE(MulAssignUnsignedInt64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, *ref *= b);
    }

    DECLARE_OPCODE(DivAssignUnsignedInt64)
    {
        auto ref  = EvalRef<uint64_t>(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, *ref /= b);
    }

    DECLARE_OPCODE(AddInt64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, a + b);
    }

    DECLARE_OPCODE(SubInt64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, a - b);
    }

    DECLARE_OPCODE(MulSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, a * b);
    }

    DECLARE_OPCODE(MulUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a - b);
    }

    DECLARE_OPCODE(DivSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, a / b);
    }

    DECLARE_OPCODE(DivUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a / b);
    }

    DECLARE_OPCODE(ModSigned64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalInt64(stack);
        RETURN(int64_t, a % b);
    }

    DECLARE_OPCODE(ModUnsigned64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a % b);
    }

    DECLARE_OPCODE(NegSigned64)
    {
        auto val = EvalInt64(stack);
        RETURN(int64_t, -val);
    }

    DECLARE_OPCODE(BitAnd64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a & b);
    }

    DECLARE_OPCODE(BitOr64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a | b);
    }

    DECLARE_OPCODE(BitXor64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint64(stack);
        RETURN(uint64_t, a ^ b);
    }

    DECLARE_OPCODE(BitNot64)
    {
        auto a = EvalUint64(stack);
        RETURN(uint64_t, ~a);
    }

    DECLARE_OPCODE(BitShl64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint8(stack);
        RETURN(uint64_t, a << b);
    }

    DECLARE_OPCODE(BitShr64)
    {
        auto a = EvalUint64(stack);
        auto b = EvalUint8(stack);
        RETURN(uint64_t, a >> b);
    }

    DECLARE_OPCODE(BitSar64)
    {
        auto a = EvalInt64(stack);
        auto b = EvalUint8(stack);
        RETURN(int64_t, a << b);
    }

    DECLARE_OPCODE(EnumToInt64)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
        RETURN(int64_t, 0);
    }

    DECLARE_OPCODE(Return)
    {
        ASSERT(stack->params()->m_returnPtr != nullptr);
        StepGeneric(stack, stack->params()->m_returnPtr);
    }
    
    DECLARE_OPCODE(ReturnDirect)
    {
        ASSERT(stack->params()->m_returnPtr != nullptr);
        StepGeneric(stack, stack->params()->m_returnPtr);
    }

    DECLARE_OPCODE(ReturnAny)
    {
        Type type = ReadPointer<const rtti::IType*>(stack);
        void* refPtr = EvalRef(stack);
        type->copy(stack->params()->m_returnPtr, refPtr);
    }

    DECLARE_OPCODE(ReturnLoad1)
    {
        auto pointerToValue = EvalRef(stack);
        *(uint8_t*)stack->params()->m_returnPtr = *(const uint8_t*)pointerToValue;
    }

    DECLARE_OPCODE(ReturnLoad2)
    {
        auto pointerToValue = EvalRef(stack);
        *(uint16_t*)stack->params()->m_returnPtr = *(const uint16_t*)pointerToValue;
    }

    DECLARE_OPCODE(ReturnLoad4)
    {
        auto pointerToValue = EvalRef(stack);
        *(uint32_t*)stack->params()->m_returnPtr = *(const uint32_t*)pointerToValue;
    }

    DECLARE_OPCODE(ReturnLoad8)
    {
        auto pointerToValue = EvalRef(stack);
        *(uint64_t*)stack->params()->m_returnPtr = *(const uint64_t*)pointerToValue;
    }

    DECLARE_OPCODE(Exit)
    {
        stack->codePtr() = stack->codeEndPtr();
    }

    DECLARE_OPCODE(Max)
    {
        ASSERT(!"OPCODE NOT IMPLEMENTED");
    }

};