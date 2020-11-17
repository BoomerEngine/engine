/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "scriptOpcodes.h"

#include "base/parser/include/textToken.h"
#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/hashSet.h"

namespace base
{
    namespace script
    {

        ///---

        struct Stub;
        struct StubModule;
        struct StubModuleImport;
        struct StubFile;
        struct StubClass;
        struct StubFunction;
        struct StubProperty;
        struct StubTypeName;
        struct StubTypeDecl;
        struct StubTypeRef;
        struct StubConstant;
        struct StubConstantValue;
        struct StubEnum;
        struct StubEnumOption;
        struct StubFunctionArg;
        struct StubOpcode;
		class FunctionCode;
        class IStubWriter;
        class IStubReader;

        enum class StubType : uint8_t
        {
            None = 0,
            Module,
            ModuleImport,
            File,
            TypeName,
            TypeDecl,
            TypeRef,
            Class,
            Constant,
            ConstantValue,
            Enum,
            EnumOption,
            Property,
            Function,
            FunctionArg,
            Opcode,

            MAX,
        };

        enum class StubFlag : uint32_t
        {
            Native,
            Import,
            State,
            Struct,
            Class,
            Explicit,
            Unsafe,
            Abstract,
            Editable,
            Protected,
            Private,
            Inlined,
            Const,
            Final,
            Static,
            Override,
            Function,
            Signal,
            Property,
            Operator,
            Cast,
            Opcode,
            Ref,
            Out,
            Constructor,
            Destructor,
            ImportDependency,
        };

        enum class StubTypeType : uint8_t
        {
            Simple, // local simple type
            Engine, // aliased engine type
            ClassType,
            PtrType,
            WeakPtrType,
            DynamicArrayType,
            StaticArrayType,
        };

        enum class StubConstValueType : uint8_t
        {
            Integer,
            Unsigned,
            Float,
            Bool,
            String,
            Name,
            Compound,
        };

        typedef BitFlags<StubFlag> StubFlags;

        struct BASE_SCRIPT_API StubLocation
        {
            const StubFile* file = nullptr;
            uint32_t line = 0;

            void print(IFormatStream& f) const;
            void write(IStubWriter& f) const;
            void read(IStubReader& f);
        };

        typedef HashSet<const Stub*> TUsedStubs;

        struct BASE_SCRIPT_API Stub : public base::NoCopy
        {
            const Stub* owner = nullptr;
            StubLocation location;
            StubFlags flags;
            StubType stubType;
            StringID name;

            virtual StringBuf fullName() const;

            virtual void write(IStubWriter& f) const = 0;
            virtual void read(IStubReader& f) = 0;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) = 0;
            virtual void postLoad();

            //--

            static Stub* Create(mem::LinearAllocator& mem, StubType type);

            //--

            virtual const StubModule* asModule() const { return nullptr; };
            virtual const StubModuleImport* asModuleImport() const { return nullptr; };
            virtual const StubFile* asFile() const { return nullptr; };
            virtual const StubClass* asClass() const { return nullptr; };
            virtual const StubFunction* asFunction() const { return nullptr; };
            virtual const StubProperty* asProperty() const { return nullptr; };
            virtual const StubTypeName* asTypeName() const { return nullptr; };
            virtual const StubTypeDecl* asTypeDecl() const { return nullptr; };
            virtual const StubTypeRef* asTypeRef() const { return nullptr; };
            virtual const StubOpcode* asOpcode() const { return nullptr; };
            virtual const StubConstant* asConstant() const { return nullptr; };
            virtual const StubConstantValue* asConstantValue() const { return nullptr; };
            virtual const StubEnum* asEnum() const { return nullptr; };
            virtual const StubEnumOption* asEnumOption() const { return nullptr; };
            virtual const StubFunctionArg* asFunctionArg() const { return nullptr; };

            virtual StubModule* asModule() { return nullptr; };
            virtual StubModuleImport* asModuleImport() { return nullptr; };
            virtual StubFile* asFile() { return nullptr; };
            virtual StubClass* asClass() { return nullptr; };
            virtual StubFunction* asFunction() { return nullptr; };
            virtual StubProperty* asProperty() { return nullptr; };
            virtual StubTypeName* asTypeName() { return nullptr; };
            virtual StubTypeDecl* asTypeDecl() { return nullptr; };
            virtual StubTypeRef* asTypeRef() { return nullptr; };
            virtual StubOpcode* asOpcode() { return nullptr; };
            virtual StubConstant* asConstant() { return nullptr; };
            virtual StubConstantValue* asConstantValue() { return nullptr; };
            virtual StubEnum* asEnum() { return nullptr; };
            virtual StubEnumOption* asEnumOption() { return nullptr; };
            virtual StubFunctionArg* asFunctionArg() { return nullptr; };
        };

        //---

        #define STUB_CLASS(x) \
        static const auto STATIC_TYPE  = StubType::x; \
        INLINE Stub##x() { stubType = StubType::x; } \
        virtual const Stub##x* as##x() const override final { return this; } \
        virtual Stub##x* as##x() override final { return this; }

        //---

        struct BASE_SCRIPT_API StubModule : public Stub
        {
            STUB_CLASS(Module);

            Array<const StubFile*> files;
            Array<const StubModule*> imports; // imported modules CLONES (note: mostly stripped of stubs)
            HashMap<StringID, const Stub*> stubMap; // map from all files in the module

            const Stub* findStub(StringID name) const;

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void postLoad() override final;

            void extractGlobalFunctions(Array<StubFunction*>& outFunctions) const;
            void extractClassFunctions(Array<StubFunction*>& outFunctions) const;
            void extractClasses(Array<StubClass*>& outClasses) const;
            void extractEnums(Array<StubEnum*>& outEnums) const;
        };

        struct BASE_SCRIPT_API StubModuleImport : public Stub
        {
            STUB_CLASS(ModuleImport);

            mutable const StubModule* m_importedModuleData = nullptr;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        struct BASE_SCRIPT_API StubFile : public Stub
        {
            STUB_CLASS(File);

            StringBuf depotPath;
            StringBuf absolutePath;

            Array<Stub*> stubs; // as parsed

            void extractGlobalFunctions(Array<StubFunction*>& outFunctions) const;
            void extractClassFunctions(Array<StubFunction*>& outFunctions) const;
            void extractClasses(Array<StubClass*>& outClasses) const;
            void extractEnums(Array<StubEnum*>& outEnums) const;

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
        };

        /// type alias name
        struct BASE_SCRIPT_API StubTypeName : public Stub
        {
            STUB_CLASS(TypeName);

            //--

            const StubTypeDecl* linkedType = nullptr;

            //--

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
        };

        /// enum option
        struct BASE_SCRIPT_API StubEnumOption : public Stub
        {
            STUB_CLASS(EnumOption);

            int64_t assignedValue = 0;
            bool hasUserAssignedValue = false;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        /// enum type
        struct BASE_SCRIPT_API StubEnum : public Stub
        {
            STUB_CLASS(Enum);

            Array<StubEnumOption*> options;
            HashMap<StringID, const StubEnumOption*> optionsMap;

            StringID engineImportName;

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void postLoad() override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;

            const StubEnumOption* findOption(StringID name) const;
        };

        /// reference to a named stub (to be resolved), always created in context
        struct BASE_SCRIPT_API StubTypeRef : public Stub
        {
            STUB_CLASS(TypeRef);

            // resolved during resolveTypes() or loaded from imported module
            // NOTE: this NEVER points to a typeDecl or typeRef or typeName, only actual types
            const Stub* resolvedStub = nullptr;

            const StubClass* classType() const;
            const StubEnum* enumType() const;

            bool isEnumType() const;
            bool isClassType() const;

            virtual StringBuf fullName() const;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;

            static bool Match(const StubTypeRef* a, const StubTypeRef* b);
        };


        /// type definition
        struct BASE_SCRIPT_API StubTypeDecl : public Stub
        {
            STUB_CLASS(TypeDecl);

            StubTypeType metaType = StubTypeType::Simple;

            const StubTypeDecl* innerType = nullptr; // arrays
            const StubTypeRef* referencedType = nullptr; // for simple, class, etc

            uint32_t arraySize = 0;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;

            bool isSharedPointerType() const;
            bool isWeakPointerType() const;
            bool isPointerType() const;
            bool isEnumType() const;
            bool isSimpleType() const;
            bool isClassType() const;

            virtual StringBuf fullName() const override;
            void printableName(IFormatStream& f) const;

            template< typename T >
            INLINE bool isType() const
            {
                if (metaType == StubTypeType::Engine)
                    return name == base::reflection::GetTypeName<T>();

                return false;
            }

            const StubClass* classType() const;
            const StubEnum* enumType() const;

            static bool Match(const StubTypeDecl* a, const StubTypeDecl* b);
        };

        /// constant value
        struct BASE_SCRIPT_API StubConstantValue : public Stub
        {
            STUB_CLASS(ConstantValue);

            StubConstValueType m_valueType = StubConstValueType::Integer;

            union
            {
                double f;
                uint64_t u;
                int64_t i;
            } value;

            StringView text; // name and string, NOTE: it's alocated from linear allocator

            const StubTypeDecl* compoundType = nullptr; // compound only
            Array<const StubConstantValue*> compoundVals;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        /// constant
        struct BASE_SCRIPT_API StubConstant : public Stub
        {
            STUB_CLASS(Constant);

            const StubTypeDecl* typeDecl = nullptr;
            const StubConstantValue* value = nullptr;

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
        };

        /// property definition
        struct BASE_SCRIPT_API StubProperty : public Stub
        {
            STUB_CLASS(Property);

            const StubTypeDecl* typeDecl = nullptr;
            const StubConstantValue* defaultValue = nullptr;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        /// function argument
        struct BASE_SCRIPT_API StubFunctionArg : public Stub
        {
            STUB_CLASS(FunctionArg);

            const StubTypeDecl* typeDecl = nullptr;
            const StubConstantValue* defaultValue = nullptr;
            short index = -1;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        /// function
        struct BASE_SCRIPT_API StubFunction : public Stub
        {
            STUB_CLASS(Function);

            StringID operatorName;
            StringID opcodeName;
            StringID aliasName;
            int castCost = 0;

            const StubTypeDecl* returnTypeDecl = nullptr;
            Array<const StubFunctionArg*> args;

            const StubFunction* baseFunction = nullptr; // matching function in base class
            const StubFunction* parentFunction = nullptr; // matching function in parent class

            base::Array<base::parser::Token*> tokens; // in case it's coming from local file (NOT SAVED)

            Array<const StubOpcode*> opcodes; // function generated opcodes
            uint64_t codeHash = 0;

            //--

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;

            const StubFunctionArg* findFunctionArg(StringID name) const;
        };

        /// class definition
        struct BASE_SCRIPT_API StubClass : public Stub
        {
            STUB_CLASS(Class);

            StringID baseClassName;
            StringID parentClassName;
            StringID engineImportName;

            Array<Stub*> stubs;
            HashMap<StringID, const Stub*> stubMap;

            const StubClass* baseClass = nullptr;
            Array<StubClass*> derivedClasses;

            const StubClass* parentClass = nullptr;
            Array<StubClass*> childClasses;

            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
            virtual void postLoad() override final;
            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;

            const Stub* findStub(StringID name) const;
            const Stub* findStubLocal(StringID name) const;

            void findAliasedFunctions(StringID name, uint32_t expectedArgumentCount, HashSet<const StubFunction*>& mutedFunctions, Array<const StubFunction*>& outAliasedFunctions) const;

            void extractFunctions(Array<StubFunction*>& outFunctions) const;
            void extractClasses(Array<StubClass*>& outClasses) const;
            void extractEnums(Array<StubEnum*>& outEnums) const;

            bool is(const StubClass* baseClass) const;
        };

        ///---

        struct BASE_SCRIPT_API StubOpcodeValue
        {
            union
            {
                uint64_t u;
                int64_t i;
                float f;
                double d;
            };

            StringID name;
            StringBuf text;

            StubOpcodeValue();
        };

        struct BASE_SCRIPT_API StubOpcode : public Stub
        {
            STUB_CLASS(Opcode);

            Opcode op = Opcode::Nop; // opcode value
            const Stub* stub = nullptr; // referenced object, constant, variable, function to call, etc
            const StubOpcode* target = nullptr; // jump target, if any
            StubOpcodeValue value; // additional value (saved only when needed)

            StubOpcode* next = nullptr; // not saved

            void print(IFormatStream& f) const;

            virtual void prune(const TUsedStubs& usedStubs, uint32_t& numRemoved) override final;
            virtual void write(IStubWriter& f) const override final;
            virtual void read(IStubReader& f) override final;
        };

        ///---

        class BASE_SCRIPT_API IStubWriter : public base::NoCopy
        {
        public:
            virtual ~IStubWriter();

            virtual void writeBool(bool b) = 0;
            virtual void writeInt8(char val) = 0;
            virtual void writeInt16(short val) = 0;
            virtual void writeInt32(int val) = 0;
            virtual void writeInt64(int64_t val) = 0;
            virtual void writeUint8(uint8_t val) = 0;
            virtual void writeUint16(uint16_t val) = 0;
            virtual void writeUint32(uint32_t val) = 0;
            virtual void writeUint64(uint64_t val) = 0;
            virtual void writeFloat(float val) = 0;
            virtual void writeDouble(double val) = 0;
            virtual void writeString(StringView str) = 0;
            virtual void writeName(StringID name) = 0;
            virtual void writeRef(const Stub* otherStub) = 0;

            template< typename T >
            INLINE void writeEnum(T val)
            {
                switch (sizeof(T))
                {
                    case 1: writeUint8((uint8_t)val); break;
                    case 2: writeUint16((uint16_t)val); break;
                    case 4: writeUint32((uint32_t)val); break;
                    case 8: writeUint64((uint64_t)val); break;
                }
            }

            template< typename T >
            INLINE void writeRefList(const Array<T*>& val)
            {
                writeUint32(val.size());
                for (auto ptr  : val)
                    writeRef(ptr);
            }
        };

        class BASE_SCRIPT_API IStubReader : public base::NoCopy
        {
        public:
            virtual ~IStubReader();

            virtual bool readBool() = 0;
            virtual char readInt8() = 0;
            virtual short readInt16() = 0;
            virtual int readInt32() = 0;
            virtual int64_t readInt64() = 0;
            virtual uint8_t readUint8() = 0;
            virtual uint16_t readUint16() = 0;
            virtual uint32_t readUint32() = 0;
            virtual uint64_t readUint64() = 0;
            virtual float readFloat() = 0;
            virtual double readDouble() = 0;
            virtual StringBuf readString() = 0; // note string are assumed to be stored inside safe memory
            virtual StringID readName() = 0;
            virtual const Stub* readRef() = 0;

            template< typename T >
            INLINE const T* readRef()
            {
                return static_cast<const T*>(readRef());
            }

            template< typename T >
            INLINE void readEnum(T& ret)
            {
                switch (sizeof(T))
                {
                    case 1: ret = (T)readUint8(); break;
                    case 2: ret = (T)readUint16(); break;
                    case 4: ret = (T)readUint32(); break;
                    case 8: ret = (T)readUint64(); break;
                }
            }

            template< typename T >
            INLINE void readRefList(Array<T*>& val)
            {
                val.resize(readUint32());

                for (uint32_t i=0; i<val.size(); ++i)
                    val[i] = (T*) readRef();
            }

            template< typename T >
            INLINE void readRef(T*& val)
            {
                val = (T*) readRef();
            }
        };

    } // script
} // base

