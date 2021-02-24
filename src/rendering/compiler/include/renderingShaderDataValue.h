/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\data #]
***/

#pragma once

#include "renderingShaderDataType.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

class WriteMask;
class ComponentMask;
struct DataParameter;
class ProgramInstance;
struct ProgramInstanceParam;

/// type of the value stored in the data value component
enum class DataValueKind : uint8_t
{
    Undefined, // value can be anything, it's not possible to know (not a const expr)
    Numerical, // numerical constant value, can be used for math
    ProgramInstance, // data for parametrized program
    Name, // StringID
};

/// a single component value, can be defined or not
/// any operation on the undefined value produces undefined value
/// the value is "type agnostic" (done with a double...)
class RENDERING_COMPILER_API DataValueComponent
{
public:
    DataValueComponent(); // undefined
    explicit DataValueComponent(float value); // DefinedAlways
    explicit DataValueComponent(uint32_t value); // DefinedAlways
    explicit DataValueComponent(int value); // DefinedAlways
    explicit DataValueComponent(bool value); // DefinedAlways
    explicit DataValueComponent(double value); // DefinedAlways
    explicit DataValueComponent(const ProgramInstance* pi); // DefinedAlways
    explicit DataValueComponent(base::StringID name); // DefinedAlways

    // is the value defined ?
    INLINE bool isDefined() const { return m_kind != DataValueKind::Undefined; }

    // is this a numerical value
    INLINE bool isNumerical() const { return m_kind == DataValueKind::Numerical; }

    // get kind of the value stored
    INLINE DataValueKind kind() const { return m_kind; }

    // get value as uint32_t (NOTE: meaningfull only in the m_constness == DefinedAlways mode
    INLINE uint32_t valueUint() const { ASSERT(m_kind == DataValueKind::Numerical); return (uint32_t)m_value; }

    // get value as int (NOTE: meaningfull only in the m_constness == DefinedAlways mode
    INLINE int valueInt() const { ASSERT(m_kind == DataValueKind::Numerical); return (int)m_value; }

    // get value as float (NOTE: meaningfull only in the m_constness == DefinedAlways mode
    INLINE float valueFloat() const { ASSERT(m_kind == DataValueKind::Numerical); return (float)m_value; }

    // get value as Boolean (NOTE: meaningfull only in the m_constness == DefinedAlways mode
    INLINE bool valueBool() const { ASSERT(m_kind == DataValueKind::Numerical); return m_value > 0.0; } // NOTE: non standard way to represent the booleans

    // get raw numerical value
    INLINE double valueNumerical() const { ASSERT(m_kind == DataValueKind::Numerical); return m_value; }

    // is this a program instance
    INLINE bool isProgramInstance() const { return (m_kind == DataValueKind::ProgramInstance); }

    // get the program instance
    INLINE const ProgramInstance* programInstance() const { ASSERT(m_kind == DataValueKind::ProgramInstance && m_programInstance); return m_programInstance; }

    // is this a name
    INLINE bool isName() const { return (m_kind == DataValueKind::Name); }

    // get the program instance
    INLINE base::StringID name() const { ASSERT(m_kind == DataValueKind::Name); return m_name; }

    //--

    // get text representation with better type representation (debug)
    void print(base::IFormatStream& f) const;

    //--

    // merge values of two operations
    static DataValueComponent Merge(const DataValueComponent& a, const DataValueComponent& b);

    // compare
    static bool Compare(const DataValueComponent& a, const DataValueComponent& b);

    //--

    // compute the CRC of the data value
    void calcCRC(base::CRC64& outCRC) const;

private:
    DataValueKind m_kind;

    union
    {
        double m_value;
        const ProgramInstance* m_programInstance;
    };

    base::StringID m_name;
};

/// a value of a node, component-wise
struct RENDERING_COMPILER_API DataValue
{
public:
    typedef base::Array<DataValueComponent> TComponents;
    TComponents m_components;

    DataValue(); // undefined and empty
    DataValue(const DataType& type); // undefined
    DataValue(const DataValue& srcValue);
    DataValue(const DataValue& srcValue, uint32_t firstElement, uint32_t numElements); // member access
    explicit DataValue(float value); // DefinedAlways
    explicit DataValue(uint32_t value); // DefinedAlways
    explicit DataValue(int value); // DefinedAlways
    explicit DataValue(bool value); // DefinedAlways
    explicit DataValue(const base::Vector2& value); // DefinedAlways
    explicit DataValue(const base::Vector3& value); // DefinedAlways
    explicit DataValue(const base::Vector4& value); // DefinedAlways
    explicit DataValue(const ProgramInstance* value); // DefinedAlways
    explicit DataValue(base::StringID value); // DefinedAlways

    // values
    INLINE bool valid() const { return !m_components.empty(); }

    // get number of components
    INLINE uint32_t size() const { return m_components.size(); }

    // get raw data
    INLINE const DataValueComponent* data() const { return m_components.typedData(); }

    // get raw data
    INLINE DataValueComponent* data() { return m_components.typedData(); }

    // dump full value in a nice way
    void print(base::IFormatStream& f) const;

    // is this scalar value ?
    bool isScalar() const;

    // is the whole value defined ?
    bool isWholeValueDefined() const;

    // get component (safe), returns undefined when out of range
    DataValueComponent component(uint32_t index = 0) const;

    // set value for component
    void component(uint32_t index, const DataValueComponent& val);

    // get matrix component (safe), returns undefined when out of range
    DataValueComponent matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row) const;

    // set matrix component, does not set if out of range
    void matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row, const DataValueComponent& val);

    //--

    // compute CRC
    void calcCRC(base::CRC64& outCRC) const;

    //--

    // merge two values, kind of like a "phi" node
    static DataValue Merge(const DataValue& a, const DataValue& b);

    // compare for equality
    static bool Compare(const DataValue& a, const DataValue& b);

    //--

    bool operator==(const DataValue& other) const;
    bool operator!=(const DataValue& other) const;
};

//--

/// a reference to a value, built using pointer to value components
class RENDERING_COMPILER_API DataValueRef
{
public:
    DataValueRef();
    DataValueRef(const DataType& type, DataValueComponent* firstComponent, uint32_t numComponents);
    DataValueRef(const DataType& type); // undefined data
    DataValueRef(const DataType& type, DataValue* value);

    // is the reference valud ?
    INLINE bool valid() const { return m_firstComponent != nullptr; }

    // is this a reference to an array ?
    INLINE bool isArray() const { return m_type.isArray(); }

    // is this a composite type (with sub-elements) ?
    INLINE bool isComposite() const { return m_type.isComposite(); }

    // get defined data type
    INLINE const DataType& dataType() const { return m_type; }

    // get number of components
    INLINE uint32_t numComponents() const { return m_numComponents; }

    // check if all of the components referenced have defined value
    bool isWholeValueDefined() const;

    // get component (safe), returns undefined when out of range
    DataValueComponent* component(uint32_t index = 0) const;

    // get array sub-element
    // NOTE: will return an invalid reference if the current reference is not an array or the index is out of range
    DataValueRef arrayElement(uint32_t index) const;

    // get the compound element reference
    // NOTE: will return an invalid reference if the current reference is not a composite type or the given element does not exist
    DataValueRef compositeElement(const base::StringID name) const;

    //--

    // read value from reference
    bool readValue(DataValue& outValue) const;

    // write value to reference
    bool writeValue(const DataValue& newValue) const;

    // write value to reference using given component mask
    bool writeValueWithMask(const DataValue& newValue, const ComponentMask& mask) const;

    //--

    // dump full value in a nice way
    void print(base::IFormatStream& f) const;

private:
    DataType m_type;
    DataValueComponent* m_firstComponent;
    uint32_t m_numComponents;
};

//--

namespace valop
{

    extern RENDERING_COMPILER_API DataValueComponent Undef();
    extern RENDERING_COMPILER_API DataValueComponent Undef(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent ToFloat(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent ToUint(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent ToInt(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent ToBool(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent Round(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Trunc(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Abs(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Sign(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Floor(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Ceil(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Frac(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent FAdd(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UAdd(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IAdd(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FSub(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent USub(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent ISub(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FMul(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UMul(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IMul(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FDiv(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UDiv(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IDiv(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FMod(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UMod(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IMod(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FNeg(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent UNeg(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent INeg(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent Lerp(const DataValueComponent& x, const DataValueComponent& y, DataValueComponent frac);

    extern RENDERING_COMPILER_API DataValueComponent Round(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Trunc(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Abs(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Sign(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Floor(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Ceil(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Frac(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent Min(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent Max(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent Clamp(const DataValueComponent& x, const DataValueComponent& minRange, const DataValueComponent& maxRange);
    extern RENDERING_COMPILER_API DataValueComponent Step(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Smoothstep(const DataValueComponent& edge0, const DataValueComponent& edge1, const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Sin(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Cos(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Tan(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Asin(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Acos(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Atan(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Atan2(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent Sinh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Cosh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Tanh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Asinh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Acosh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Atanh(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Pow(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent Log(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Exp(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Log2(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Exp2(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Sqrt(const DataValueComponent& x);
    extern RENDERING_COMPILER_API DataValueComponent Rsqrt(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent BitwiseOr(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent BitwiseXor(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent BitwiseAnd(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent BitwiseNot(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent NameEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent NameNotEqual(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent LogicalEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalNotEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalOr(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalAnd(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalXor(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalNot(const DataValueComponent& x);

    extern RENDERING_COMPILER_API DataValueComponent LogicalShiftLeft(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent LogicalShiftRight(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent ArithmeticShiftRight(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent IntEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IntNotEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IntLess(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IntLessEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IntGreater(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent IntGreaterEqual(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent UintEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UintNotEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UintLess(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UintLessEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UintGreater(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent UintGreaterEqual(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedNotEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedLess(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedLessEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedGreater(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatOrderedGreaterEqual(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedNotEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedLess(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedLessEqual(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedGreater(const DataValueComponent& x, const DataValueComponent& y);
    extern RENDERING_COMPILER_API DataValueComponent FloatUnorderedGreaterEqual(const DataValueComponent& x, const DataValueComponent& y);

    extern RENDERING_COMPILER_API DataValueComponent Select(const DataValueComponent& selector, const DataValueComponent& x, const DataValueComponent& y);

    //---

    extern RENDERING_COMPILER_API DataValue MaskRead(const DataValue& currentValue, const ComponentMask& readSwizzle);

    //---

} // valop

//--

END_BOOMER_NAMESPACE(rendering::shadercompiler)
