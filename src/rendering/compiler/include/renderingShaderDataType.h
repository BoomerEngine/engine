/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\data #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

class CompositeType;
class FunctionSignature;

//----

// flags for the type
enum class TypeFlags : uint8_t
{
    Pointer = FLAG(0), // pointer to a location in memory that can be loaded/stored
    Atomic = FLAG(1), // pointer supports atomic operations
    TEST_MASK = Pointer,
};

//---

/// shader data type
enum class BaseType : uint8_t
{
    Invalid, // error type - returned to signal error, a little bit of abuse
    Void, // a void type, used 
    Float, // 32-bit floating point scalar
    Int, // 32-bit signed integer scalar
    Uint, // 32-bit unsigned integer scalar
    Boolean, // boolean type (represented as 32-bit unsigned value with 0=false !=0=true)
    Struct, // structure from the type library

    Program, // parametrized instance of a program
    Name, // named option (permutations only)
    Function, // function signature
    Resource, // resource (Texture, Buffer, etc)
};

//---

/// array size description (actual element counts)
class RENDERING_COMPILER_API ArrayCounts
{
public:
    static const uint8_t MAX_ARRAY_DIMS = 4;
    static const short COUNT_NOT_DEFINED = -1;

    ArrayCounts(); // no array
    ArrayCounts(uint32_t dim);
    ArrayCounts(const ArrayCounts& other);

    // empty array ? (no elements)
    INLINE bool empty() const { return m_sizes[0] == 0; }

    // is this array full ? (no more dimensions allowed)
    INLINE bool full() const { return m_sizes[MAX_ARRAY_DIMS-1] != 0; }

    // are all array counts defined ?
    bool isSizeDefined() const;

    // get number of array dimmensions (0 == scalar)
    uint8_t dimensionCount() const;

    // count all array elements
    // fails if the array count is not defined
    uint32_t elementCount() const;

    // get inner array size, will return empty array once reached the scalar level
    // NOTE: fatal asserts if called on empty array
    ArrayCounts innerCounts() const;

    // get the outer most array dimensions
    // returns 1 in case of scalars (for simplicity)
    // fails if the array count is not defined
    uint32_t outermostCount() const;

    // get number of array elements at given dimension, can return COUNT_NOT_DEFINED if array information does not contain that data
    // the dimmension must be within the array limits so this function should not be called for empty arrays
    // never returns 0
    int arraySize(uint32_t dimmension = 0) const;

    //--

    // create a new array count by prepending undefined range at THE FRONT
    // array -> []array
    // fails if the array dimmensions == MAX_ARRAY_DIMS
    ArrayCounts prependUndefinedArray() const;

    // create a new array count by appending undefined range at THE BACK
    // array -> array[]
    // fails if the array dimmensions == MAX_ARRAY_DIMS
    ArrayCounts appendUndefinedArray() const;

    // create a new array count by prepending defined amount of elements at THE FRONT 
    // array -> []array
    // fails if the array dimmensions == MAX_ARRAY_DIMS or if the count it zero
    ArrayCounts prependArray(uint32_t count) const;

    // create a new array count by appending undefined range at THE FRONT
    // array -> array[]
    // fails if the array dimmensions == MAX_ARRAY_DIMS
    ArrayCounts appendArray(uint32_t count) const;

    //--

    // get a unique type hash
    uint64_t typeHash() const;

    //--

    // compare types for strict equality, all counts must match
    static bool StrictMatch(const ArrayCounts& a, const ArrayCounts& b);

    // compare types for general equality (number of dimensions equal, counts can be dropped)
    static bool GeneralMatch(const ArrayCounts& a, const ArrayCounts& b);

    //--

    // get a string representation (for debug and error printing)
    void print(base::IFormatStream& f) const;

	//--

	// compare
	bool operator==(const ArrayCounts& other) const;
	bool operator!=(const ArrayCounts& other) const;

private:
    int m_sizes[MAX_ARRAY_DIMS];
};

//---

/// resource type
struct RENDERING_COMPILER_API ResourceType
{
	DeviceObjectViewType type = DeviceObjectViewType::Invalid;

    AttributeList attributes; // additional attributes

    const CompositeType* resolvedLayout = nullptr; // resolved layout for composited resources (structured buffers etc)
    ImageFormat resolvedFormat = ImageFormat::UNKNOWN; // resolved format
    ImageViewType resolvedViewType = ImageViewType::View2D; // resolved "size" of the texture view
	BaseType sampledImageFlavor = BaseType::Invalid;

    bool depth = false; // depth sampler
    bool multisampled = false; // MS versions

    INLINE ResourceType() {};

    bool calcTypeHash(base::CRC64& crc) const;

    uint8_t dimensions() const;
    uint8_t addressComponentCount() const;
    uint8_t sizeComponentCount() const;
    uint8_t offsetComponentCount() const;

    void print(base::IFormatStream& f) const;
};

//---

/// shader data type
struct RENDERING_COMPILER_API DataType
{
public:
    DataType();
    DataType(BaseType baseType);
    DataType(const DataType& baseType) = default;
    DataType(const CompositeType* composite);
    DataType(const Function* func);
    DataType(const Program* program);
    DataType(const ResourceType* res);

    // get base type
    INLINE BaseType baseType() const
    {
        return m_baseType;
    }

    // is this a valid type ?
    INLINE bool valid() const
    {
        return (m_baseType != BaseType::Invalid);
    }

    // is this a scalar type?
    // does not check for array count
    INLINE bool isScalar() const
    {
        return !isArray() && !m_composite;
    }

    // is this a composite type ?
    INLINE bool isComposite() const
    {
        return (m_baseType == BaseType::Struct);
    }

    // is this a pointer to some memory with value ?
    // NOTE: pointers are not explicit and are internal SPIR-V crap
    // the whole point is that when we have a pointer and require non pointer we must do a implicit load
    // also, the right-hand side operations like Store requires a pointer to work
    INLINE bool isReference() const
    {
        return m_flags.test(TypeFlags::Pointer);
    }

    // is this atomic capable memory location ?
    INLINE bool isAtomic() const
    {
        return m_flags.test(TypeFlags::Atomic);
    }

    // is this a numerical type ? (float/int)
    INLINE bool isNumericalScalar() const
    {
        return (m_baseType == BaseType::Float) || (m_baseType == BaseType::Int) || (m_baseType == BaseType::Uint);
    }

	// is this a numerical integer type ? (uint/int)
	INLINE bool isArrayIndex() const
	{
		return (m_baseType == BaseType::Int) || (m_baseType == BaseType::Uint);
	}

    // is this a name type ?
    INLINE bool isName() const
    {
        return (m_baseType == BaseType::Name);
    }

    // is this a boolean type ?
    INLINE bool isBoolean() const
    {
        return (m_baseType == BaseType::Boolean);
    }

    // is this an array ?
    INLINE bool isArray() const
    {
        return !m_arrayCounts.empty();
    }

    // is this an aggregate (array or struct)
    INLINE bool isAggregate() const
    {
        return isArray() || isComposite();
    }

    // is this a sampler texture ?
    INLINE bool isResource() const
    {
        return (m_baseType == BaseType::Resource);
    }

    // is this a function type ?
    INLINE bool isFunction() const
    {
        return (m_baseType == BaseType::Function);
    }

    // is this a program type ?
    INLINE bool isProgram() const
    {
        return (m_baseType == BaseType::Program);
    }

    // get custom flags
    INLINE const base::DirectFlags<TypeFlags>& flags() const
    {
        return m_flags;
    }

    // get the array counts (number of element of array for each dimension)
    INLINE const ArrayCounts& arrayCounts() const
    {
        return m_arrayCounts;
    }

    // get number of array dimensions
    INLINE uint8_t arrayDimensionCount() const
    {
        return m_arrayCounts.dimensionCount();
    }

    // get the referenced composite type
    INLINE const CompositeType& composite() const
    {
        ASSERT(m_composite != nullptr);
        return *m_composite;
    }

    // resource declaration
    INLINE const ResourceType& resource() const
    {
        ASSERT(m_resource != nullptr);
        return *m_resource;
    }

    // get the program for the program type
    INLINE const Program* program() const
    {
        ASSERT(m_baseType == BaseType::Program);
        return m_program;
    }

    // get the function for the function type
    INLINE const Function* function() const
    {
        ASSERT(m_baseType == BaseType::Function);
        return m_function;
    }

    //--

    // compare types for equality without checking the flags
    static bool MatchNoFlags(const DataType& a, const DataType& b);

    //--

    // get a unique type hash
    bool calcTypeHash(base::CRC64& crc) const;

    // is this a vector type (composite with 2-4 components)
    bool isVector() const;

    // is this a matrix type?
    bool isMatrix() const;

    // check if we are a numerical vector type (scalars count at 1D vectors)
    bool isNumericalVectorLikeOperand(bool allowBool = false) const;

    // check if we are a matrix like operand, row count > 1 is required
    bool isNumericalMatrixLikeOperand() const;

    //--

    // convert to reference and back
    DataType makePointer() const;
    DataType unmakePointer() const;

    // convert to atomic type and back
    DataType makeAtomic() const;
    DataType unmakeAtomic() const;

    // apply new flags to type
    DataType applyFlags(const base::DirectFlags<TypeFlags>& flags) const;

    // apply new array counts to type, usually used to specialize the array
    DataType applyArrayCounts(const ArrayCounts& arrayCounts) const;

    // remove array counts from type
    DataType removeArrayCounts() const;

    //--

    // get boolean scalar type
    static DataType BoolScalarType();

    // get name scalar type
    static DataType NameScalarType();

    // get integer scalar type
    static DataType IntScalarType();

    // get unsigned scalar type
    static DataType UnsignedScalarType();

    // get float scalar type
    static DataType FloatScalarType();

    // get the generic program type
    static DataType ProgramType();

    //--

    // compute number of SCALAR components required to represent value of this type
    // note: this is recursive, takes the composite types into account
    // note: pointers are counted as 1 scalar component since they cannot be directly represented anyway
    uint32_t computeScalarComponentCount() const;

    //--

    // get a string representation (for debug and error printing)
    void print(base::IFormatStream& f) const;

	//--

	// compare types
	bool operator==(const DataType& other) const;
	bool operator!=(const DataType& other) const;

	// calculate type hash
	static uint32_t CalcHash(const DataType& type);

private:
    BaseType m_baseType; // base kind of the type
    base::DirectFlags<TypeFlags> m_flags; // internal flags
    ArrayCounts m_arrayCounts; // array element counts (if known)

	// TODO: union!
	const CompositeType* m_composite = nullptr;
	const Function* m_function = nullptr;
	const Program* m_program = nullptr;
	const ResourceType* m_resource = nullptr;
};

END_BOOMER_NAMESPACE(rendering::shadercompiler)
