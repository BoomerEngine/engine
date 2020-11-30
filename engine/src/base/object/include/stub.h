/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: stubs #]
***/

#pragma once

namespace base
{
	//---

	// a basic stub, a light-weight object mean for serializing very numerous but simple data structures (AST in scripts mostly) in a convenient way
	// NOTE: stubs are never allocated as separate memory allocations and always live inside some kind of memory pool (linear allocator usually)
	// NOTE: it's assumed that stub's wont need destruction (destroying 10000s of small objects can take a lot of time)
	// NOTE: it's also assumed that loaded stubs are READ ONLY
	struct BASE_OBJECT_API IStub : public NoCopy
	{
		RTTI_DECLARE_POOL(POOL_STUBS);

	public:
		IStub();
		virtual ~IStub();

		//--

		virtual const char* debugName() const = 0;

		static StubTypeValue StaticType() { return 0; }
		virtual StubTypeValue runtimeType() const = 0;

		virtual void write(IStubWriter& f) const = 0;
		virtual void read(IStubReader& f) = 0; // NOTE: it's not safe to access other stubs here yet
		virtual void postLoad(); // called after all stubs finish loading

		//--

		// save this stub (and all other referenced stubs) to a packed and compressed memory blob
		Buffer pack(uint32_t version) const;

		//--
	};

	//---

#define STUB_CLASS(x) \
	static base::StubTypeValue StaticType() { return (base::StubTypeValue)(StubType::##x); } \
	virtual base::StubTypeValue runtimeType() const override { return (base::StubTypeValue)(StubType::##x); } \
	virtual const char* debugName() const override { return #x; } \
    virtual const Stub##x* as##x() const override final { return this; } \
    virtual Stub##x* as##x() override final { return this; }

	//---

	// since stubs don't have destructor in general but lots of them need to store arrays of some kind this pseudo array is here mainly to provide the "foreach" iteration :)
	template< typename T >
	struct StubPseudoArray : public NoCopy
	{
		uint32_t elemCount = 0;
		const T* const* elems = nullptr;

		INLINE StubPseudoArray() {};
		INLINE uint32_t size() const { return elemCount; }
		INLINE bool empty() const { return elemCount == 0; }
		INLINE const T* operator[](uint32_t index) const { return elems[index]; }

		INLINE StubPseudoArray(StubPseudoArray<T>&& other)
		{
			elemCount = other.elemCount;
			elems = other.elems;
			other.elems = nullptr;
			other.elemCount = 0;
		}

		INLINE StubPseudoArray& operator=(StubPseudoArray<T>&& other)
		{
			if (this != &other)
			{
				elemCount = other.elemCount;
				elems = other.elems;
				other.elems = nullptr;
				other.elemCount = 0;
			}

			return *this;
		}

		INLINE ConstArrayIterator<const T*> begin() const { return elems; }
		INLINE ConstArrayIterator<const T*> end() const { return elems + elemCount; }

		INLINE ArrayIterator<const T*> begin() { return elems; }
		INLINE ArrayIterator<const T*> end() { return elems + elemCount; }
	};

	//---

	struct StubHeader
	{
		static const uint32_t MAGIC = 'STUB';
		
		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t uncompressedSize = 0;
		uint32_t additionalDataSize = 0;
		uint32_t compressedCRC = 0;

		uint32_t numStrings = 0;
		uint32_t numNames = 0;
		uint32_t numStubs = 0;
	};

	//---

	// abstract writer of stub data  
	class BASE_OBJECT_API IStubWriter : public NoCopy
	{
	public:
		virtual ~IStubWriter();

		// simple data types, separate methods are mostly here to aid in self documenting code
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

		// write compressed number (great for indexes)
		virtual void writeCompressedInt(int val) = 0;

		// indexed string
		virtual void writeString(StringView str) = 0;

		// indexed name
		virtual void writeName(StringID name) = 0;

		// reference to other stub
		virtual void writeRef(const IStub* otherStub) = 0;

		// write list of references (remembers needed memory size)
		virtual void writeRefList(const IStub* const* otherStubs, uint32_t numRefs) = 0;

		// raw data
		virtual void writeData(const void* data, uint32_t size) = 0;

		// enumerator value (saved as smallest required type)
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

		// pseudo (inlined) array
		template< typename T >
		INLINE void writeArray(const StubPseudoArray<T>& val)
		{
			writeRefList((const base::IStub* const*) val.elems, val.size());
		}
	};

	// abstract loader/unpacker of stub data
	class BASE_OBJECT_API IStubReader : public NoCopy
	{
	public:
		IStubReader(uint32_t version);
		virtual ~IStubReader();

		INLINE const uint32_t version() const { return m_version; }

		// simple types, separate methods are mostly here to aid in self documenting code
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

		// variable length encoding number, good to save space on small values
		virtual int readCompressedInt() = 0;

		// indexed string, returns pointer to memory inside the UnpackedStubs class
		virtual StringView readString() = 0;

		// indexed name
		virtual StringID readName() = 0;

		// reference to other stub, fails if type does not match
		virtual const IStub* readRef(StubTypeValue expectedType) = 0;

		// list of references (pointer is inside the UnpackedStubs class)
		virtual const IStub** readRefList(uint32_t count) = 0;

		// arbitrary data, again pointer will be owned by UnpackedStubs class
		virtual const void* readData(uint32_t size) = 0;

		// typed reference
		template< typename T >
		INLINE const T* readRef()
		{
			return static_cast<const T*>(readRef());
		}

		// enum (auto select best size)
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
		INLINE void readRef(const T*& val)
		{
			val = (const T*)readRef(T::StaticType());
		}

		template< typename T >
		INLINE void readArray(StubPseudoArray<T>& val)
		{
			val.elemCount = readCompressedInt();
			val.elems = (const T* const*)readRefList(val.elemCount);
		}

	private:
		uint32_t m_version = 0;
	};

	//---

} // base