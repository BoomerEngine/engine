/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\data #]
***/

#include "build.h"
#include "renderingShaderDataValue.h"
#include "core/containers/include/stringBuilder.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderFunction.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

DataValueComponent::DataValueComponent()
    : m_kind(DataValueKind::Undefined)
    , m_value(0.0f)
{}

DataValueComponent::DataValueComponent(double value)
    : m_kind(DataValueKind::Numerical)
    , m_value(value)
{}

DataValueComponent::DataValueComponent(float value)
    : m_kind(DataValueKind::Numerical)
    , m_value(range_cast<double>(value))
{}

DataValueComponent::DataValueComponent(uint32_t value)
    : m_kind(DataValueKind::Numerical)
    , m_value(range_cast<double>(value))
{}

DataValueComponent::DataValueComponent(int value)
    : m_kind(DataValueKind::Numerical)
    , m_value(range_cast<double>(value))
{}

DataValueComponent::DataValueComponent(bool value)
    : m_kind(DataValueKind::Numerical)
    , m_value(value ? 1.0 : 0.0)
{}

DataValueComponent::DataValueComponent(StringID name)
    : m_kind(DataValueKind::Name)
    , m_name(name)
    , m_value(0.0)
{}

DataValueComponent::DataValueComponent(const ProgramInstance* pi)
    : m_kind(DataValueKind::ProgramInstance)
    , m_programInstance(pi)
{
    DEBUG_CHECK_EX(pi != nullptr, "Null programm instance passed");
}

void DataValueComponent::print(IFormatStream& f) const
{
    switch (m_kind)
    {
        case DataValueKind::Numerical:
        {
            f << m_value;
            break;
        }

        case DataValueKind::Name:
        {
            f << m_name;
            break;
        }

        case DataValueKind::ProgramInstance:
        {
            f.appendf("shader<{}> {", m_programInstance->program()->name());
            f << m_programInstance->params();
            f << "}";
            break;
        }

        default: f << "undefined"; break;
    }
}

DataValueComponent DataValueComponent::Merge(const DataValueComponent& a, const DataValueComponent& b)
{
    DataValueComponent ret;

    if (a.m_kind == a.m_kind)
    {
        if (a.m_value == b.m_value && a.m_name == b.m_name)
        {
            ret.m_kind = a.m_kind;
            ret.m_value = a.m_value;
            ret.m_name = a.m_name;
        }
        else
        {
            ret.m_kind = DataValueKind::Undefined;
            ret.m_value = 0.0f;
            ret.m_name = StringID();
        }
    }

    return ret;
}

bool DataValueComponent::Compare(const DataValueComponent& a, const DataValueComponent& b)
{
    if (a.m_kind == b.m_kind)
        if (a.m_value != b.m_value || a.m_name != b.m_name)
            return false;

    return true;
}

void DataValueComponent::calcCRC(CRC64& outCRC) const
{
    outCRC << (uint8_t)m_kind;
    if (m_kind == DataValueKind::Numerical)
        outCRC << m_value;
    else if (m_kind == DataValueKind::Name)
        outCRC << m_name.c_str();
    else if (m_kind == DataValueKind::ProgramInstance)
        outCRC << m_programInstance->key();
}

//---

namespace valop
{
    DataValueComponent Undef()
    {
        return DataValueComponent();
    }

    DataValueComponent Undef(const DataValueComponent& x, const DataValueComponent& y)
    {
        return DataValueComponent();
    }

    DataValueComponent ToFloat(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return x;
    }

    DataValueComponent ToUint(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        auto valMin = (double)0.0f;
        auto valMax = (double)std::numeric_limits<uint32_t>::max();
        return DataValueComponent((uint32_t)std::min(valMax, std::max(valMin, x.valueNumerical())));
    }

    DataValueComponent ToInt(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        auto valMin = (double)std::numeric_limits<int>::min();
        auto valMax = (double)std::numeric_limits<int>::max();
        return DataValueComponent((int)std::min(valMax, std::max(valMin, x.valueNumerical())));
    }

    DataValueComponent ToBool(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(x.valueNumerical() != 0.0);
    }

    DataValueComponent FNeg(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent((float)-x.valueFloat());
    }

    DataValueComponent INeg(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent((int)-x.valueInt());
    }

    DataValueComponent UNeg(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent((uint32_t)-x.valueInt());
    }

    DataValueComponent Lerp(const DataValueComponent& x, const DataValueComponent& y, DataValueComponent frac)
    {
        if (!frac.isNumerical()) return Undef();
        if (!x.isNumerical()) return Undef();
        if (!y.isNumerical()) return Undef();

        if (frac.valueFloat() == 0.0f)
            return x;
        else if (frac.valueFloat() == 1.0f)
            return y;

        return FAdd(x, FMul(FSub(y, x), frac));
    }

    DataValueComponent Round(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::round(x.valueFloat()));
    }

    DataValueComponent Trunc(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::trunc(x.valueFloat()));
    }

    DataValueComponent Abs(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() >= 0.0f)
            return DataValueComponent(x);
        else
            return DataValueComponent(-x.valueNumerical());
    }

    DataValueComponent Sign(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() > 0.0f)
            return DataValueComponent(1.0f);
        else if (x.valueNumerical() < 0.0f)
            return DataValueComponent(-1.0f);
        else
            return DataValueComponent(0.0f);
    }

    DataValueComponent Floor(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::floor(x.valueFloat()));
    }

    DataValueComponent Ceil(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::ceil(x.valueFloat()));
    }

    DataValueComponent Frac(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() - std::trunc(x.valueFloat()));
    }

    DataValueComponent Min(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(std::min<double>(x.valueNumerical(), x.valueNumerical()));
    }

    DataValueComponent Max(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(std::max<double>(x.valueNumerical(), x.valueNumerical()));
    }

    DataValueComponent Clamp(const DataValueComponent& x, const DataValueComponent& minRange, const DataValueComponent& maxRange)
    {
        return Min(Max(x, minRange), maxRange);
    }

    DataValueComponent Step(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return x.valueNumerical() >= 0.0f ? DataValueComponent(1.0f) : DataValueComponent(0.0f);
    }

    DataValueComponent Smoothstep(const DataValueComponent& edge0, const DataValueComponent& edge1, const DataValueComponent& x)
    {
        auto t = FDiv(FSub(x, edge0), FSub(edge1, edge0));
        if (!t.isNumerical())
            return Undef();
        auto ct = Clamp(t, DataValueComponent(0.0f), DataValueComponent(1.0));
        return FMul(FMul(t, t), FSub(DataValueComponent(3.0f), FMul(DataValueComponent(2.0f), t)));
    }

    DataValueComponent Sin(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::sin(x.valueFloat()));
    }

    DataValueComponent Cos(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::cos(x.valueFloat()));
    }

    DataValueComponent Tan(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueFloat() <= -HALFPI || x.valueFloat() >= HALFPI) return Undef();
        return DataValueComponent(std::tan(x.valueFloat()));
    }

    DataValueComponent Asin(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueFloat() < -1.0f || x.valueFloat() > -1.0f) return Undef();
        return DataValueComponent(std::asin(x.valueFloat()));
    }

    DataValueComponent Acos(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueFloat() < -1.0f || x.valueFloat() > -1.0f) return Undef();
        return DataValueComponent(std::acos(x.valueFloat()));
    }

    DataValueComponent Atan(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::atan(x.valueFloat()));
    }

    DataValueComponent Atan2(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(std::atan2(x.valueFloat(), y.valueFloat()));
    }

    DataValueComponent Sinh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::sinh(x.valueFloat()));
    }

    DataValueComponent Cosh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::cosh(x.valueFloat()));
    }

    DataValueComponent Tanh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        // TODO: range check
        return DataValueComponent(std::tanh(x.valueFloat()));
    }

    DataValueComponent Asinh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        // TODO: range check 
        return DataValueComponent(std::asinh(x.valueFloat()));
    }

    DataValueComponent Acosh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        // TODO: range check 
        return DataValueComponent(std::acosh(x.valueFloat()));
    }

    DataValueComponent Atanh(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        // TODO: range check 
        return DataValueComponent(std::tanh(x.valueFloat()));
    }

    DataValueComponent Pow(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();

        if (y.valueFloat() == 0.0f)
            return DataValueComponent(1.0f);

        if (y.valueNumerical() < 1.0f && x.valueNumerical() < 0.0f) return Undef();
        return DataValueComponent(std::pow(x.valueFloat(), y.valueFloat()));
    }

    DataValueComponent Log(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() <= 0.0f) return Undef();
        return DataValueComponent(std::log(x.valueFloat()));
    }

    DataValueComponent Exp(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::exp(x.valueFloat()));
    }

    DataValueComponent Log2(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() <= 0.0f) return Undef();
        return DataValueComponent(std::log2(x.valueFloat()));
    }

    DataValueComponent Exp2(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(std::pow(x.valueFloat(), 2.0f));
    }

    DataValueComponent Sqrt(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() < 0.0f) return Undef();
        return DataValueComponent(std::sqrt(x.valueFloat()));
    }

    DataValueComponent Rsqrt(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        if (x.valueNumerical() <= 0.0f) return Undef();
        return DataValueComponent(1.0f / std::sqrt(x.valueFloat()));
    }

    DataValueComponent FAdd(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() + y.valueFloat());
    }

    DataValueComponent UAdd(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() + y.valueUint());
    }

    DataValueComponent IAdd(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() + y.valueInt());
    }

    DataValueComponent FSub(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() - y.valueFloat());
    }

    DataValueComponent USub(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() - y.valueUint());
    }

    DataValueComponent ISub(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() - y.valueInt());
    }

    DataValueComponent FMul(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() * y.valueFloat());
    }

    DataValueComponent UMul(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() * y.valueUint());
    }

    DataValueComponent IMul(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() * y.valueInt());
    }

    DataValueComponent FDiv(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueFloat() == 0.0f) return Undef();
        return DataValueComponent(x.valueFloat() / y.valueFloat());
    }

    DataValueComponent UDiv(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueUint() == 0) return Undef();
        return DataValueComponent(x.valueUint() / y.valueUint());
    }

    DataValueComponent IDiv(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueInt() == 0) return Undef();
        return DataValueComponent(x.valueInt() / y.valueInt());
    }

    DataValueComponent FMod(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueFloat() == 0.0f) return Undef();
        return DataValueComponent(fmodf(x.valueFloat(), y.valueFloat()));
    }

    DataValueComponent UMod(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueUint() == 0) return Undef();
        return DataValueComponent(x.valueUint() % y.valueUint());
    }

    DataValueComponent IMod(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        if (y.valueInt() == 0) return Undef();
        return DataValueComponent(x.valueInt() % y.valueInt());\
    }

    DataValueComponent BitwiseOr(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() | y.valueUint());
    }

    DataValueComponent BitwiseXor(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() ^ y.valueUint());
    }

    DataValueComponent BitwiseAnd(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() & y.valueUint());
    }

    DataValueComponent BitwiseNot(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(~x.valueUint());
    }

    DataValueComponent LogicalShiftLeft(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();

        if (y.isDefined())
        {
            if (y.valueUint() == 0)
                return x; // no shift
            else if (y.valueUint() == 32)
                return DataValueComponent(0); // shift to zero
        }

        return DataValueComponent(x.valueUint() << y.valueUint());
    }

    DataValueComponent LogicalShiftRight(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();

        if (y.isDefined())
        {
            if (y.valueUint() == 0)
                return x; // no shift
            else if (y.valueUint() == 32)
                return DataValueComponent(0); // shift to zero
        }

        return DataValueComponent(x.valueUint() >> y.valueUint());
    }

    DataValueComponent ArithmeticShiftRight(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();

        if (y.isDefined())
        {
            if (y.valueUint() == 0)
                return x; // no shift
            else if (y.valueUint() == 32)
                return DataValueComponent(0); // shift to zero
        }

        return DataValueComponent(x.valueInt() >> y.valueInt());
    }

    DataValueComponent NameEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isName() || !y.isName()) return Undef();
        return DataValueComponent(x.name() == y.name());
    }

    DataValueComponent LogicalEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueBool() == y.valueBool());
    }

    DataValueComponent NameNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isName() || !y.isName()) return Undef();
        return DataValueComponent(x.name() != y.name());
    }

    DataValueComponent LogicalNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueBool() != y.valueBool());
    }

    DataValueComponent LogicalOr(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueBool() | y.valueBool());
    }

    DataValueComponent LogicalAnd(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueBool() & y.valueBool());
    }

    DataValueComponent LogicalXor(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueBool() ^ y.valueBool());
    }

    DataValueComponent LogicalNot(const DataValueComponent& x)
    {
        if (!x.isNumerical()) return Undef();
        return DataValueComponent(!x.valueBool());
    }

    DataValueComponent Select(const DataValueComponent& selector, const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!selector.isNumerical()) return Undef();
        return selector.valueBool() ? x : y;
    }

    DataValueComponent IntEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() == y.valueInt());
    }

    DataValueComponent IntNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() != y.valueInt());
    }

    DataValueComponent IntLess(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() < y.valueInt());
    }

    DataValueComponent IntLessEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() <= y.valueInt());
    }

    DataValueComponent IntGreater(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() > y.valueInt());
    }

    DataValueComponent IntGreaterEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueInt() >= y.valueInt());
    }

    DataValueComponent UintEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() == y.valueUint());
    }

    DataValueComponent UintNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() != y.valueUint());
    }

    DataValueComponent UintLess(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() < y.valueUint());
    }

    DataValueComponent UintLessEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() <= y.valueUint());
    }

    DataValueComponent UintGreater(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() > y.valueUint());
    }

    DataValueComponent UintGreaterEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueUint() >= y.valueUint());
    }

    //--

    DataValueComponent FloatOrderedEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() == y.valueFloat());
    }

    DataValueComponent FloatOrderedNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() != y.valueFloat());
    }

    DataValueComponent FloatOrderedLess(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() < y.valueFloat());
    }

    DataValueComponent FloatOrderedLessEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() <= y.valueFloat());
    }

    DataValueComponent FloatOrderedGreater(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() > y.valueFloat());
    }

    DataValueComponent FloatOrderedGreaterEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() >= y.valueFloat());
    }

    //--

    DataValueComponent FloatUnorderedEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() == y.valueFloat());
    }

    DataValueComponent FloatUnorderedNotEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() != y.valueFloat());
    }

    DataValueComponent FloatUnorderedLess(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() < y.valueFloat());
    }

    DataValueComponent FloatUnorderedLessEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() <= y.valueFloat());
    }

    DataValueComponent FloatUnorderedGreater(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() > y.valueFloat());
    }

    DataValueComponent FloatUnorderedGreaterEqual(const DataValueComponent& x, const DataValueComponent& y)
    {
        if (!x.isNumerical() || !y.isNumerical()) return Undef();
        return DataValueComponent(x.valueFloat() >= y.valueFloat());
    }

    //--

    DataValue MaskRead(const DataValue& currentValue, const ComponentMask& componentMask)
    {
        DataValue ret;

        auto numComponents = componentMask.numberOfComponentsProduced();
        ret.m_components.resize(numComponents);

        for (uint32_t i = 0; i < numComponents; ++i)
        {
            auto& retComponent = ret.m_components[i];

            auto componentSwizzle = componentMask.componentSwizzle(i);
            switch (componentSwizzle)
            {
                case ComponentMaskBit::NotDefined: retComponent = DataValueComponent(); break;
                case ComponentMaskBit::Zero: retComponent = DataValueComponent(0.0f); break;
                case ComponentMaskBit::One: retComponent = DataValueComponent(1.0f); break;
                case ComponentMaskBit::X: retComponent = currentValue.component(0);  break;
                case ComponentMaskBit::Y: retComponent = currentValue.component(1);  break;
                case ComponentMaskBit::Z: retComponent = currentValue.component(2);  break;
                case ComponentMaskBit::W: retComponent = currentValue.component(3);  break;
            }
        }

        return ret;
    }

    DataValue MaskWrite(const DataValue& currentValue, const DataValue& newValue, const ComponentMask& componentMask)
    {
        if (!componentMask.isMask() || newValue.m_components.size() >= 4)
            return newValue;

        DataValue ret = currentValue;

        ComponentMask::TWriteList writeList;
        componentMask.componentsToWrite(writeList);
        for (uint32_t i = 0; i < writeList.size(); ++i)
        {
            auto destCompIndex = writeList[i];
            ASSERT(destCompIndex < ret.m_components.size());
            ret.m_components[destCompIndex] = newValue.m_components[i];
        }

        return ret;
    }

}

//---

DataValue::DataValue()
{}

DataValue::DataValue(const DataType& type)
{
    auto numElements = type.computeScalarComponentCount();
    m_components.resize(numElements);

    for (auto& comp : m_components)
        comp = DataValueComponent();
}

DataValue::DataValue(const DataValue& srcValue)
{
    m_components = srcValue.m_components;
}

DataValue::DataValue(const DataValue& srcValue, uint32_t firstElement, uint32_t numElements)
{
    m_components.resize(numElements);
    for (uint32_t i = 0; i < numElements; ++i)
        m_components[i] = srcValue.component(firstElement + i);
}

DataValue::DataValue(float value)
{
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

DataValue::DataValue(uint32_t value)
{
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

DataValue::DataValue(int value)
{
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

DataValue::DataValue(bool value)
{
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

DataValue::DataValue(const Vector2& value)
{
    m_components.resize(2);
    m_components[0] = DataValueComponent(value.x);
    m_components[1] = DataValueComponent(value.y);
}

DataValue::DataValue(const Vector3& value)
{
    m_components.resize(3);
    m_components[0] = DataValueComponent(value.x);
    m_components[1] = DataValueComponent(value.y);
    m_components[2] = DataValueComponent(value.z);
}

DataValue::DataValue(const Vector4& value)
{
    m_components.resize(4);
    m_components[0] = DataValueComponent(value.x);
    m_components[1] = DataValueComponent(value.y);
    m_components[2] = DataValueComponent(value.z);
    m_components[3] = DataValueComponent(value.w);
}

DataValue::DataValue(const ProgramInstance* value)
{
    DEBUG_CHECK_EX(value != nullptr, "Null programm instance passed");
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

DataValue::DataValue(StringID value)
{
    m_components.resize(1);
    m_components[0] = DataValueComponent(value);
}

bool DataValue::isScalar() const
{
    return m_components.size() == 1;
}

bool DataValue::isWholeValueDefined() const
{
    if (m_components.empty())
        return false;

    for (auto& val : m_components)
        if (!val.isDefined())
            return false;

    return true;
}

DataValueComponent DataValue::component(const uint32_t index /*= 0*/) const
{
    if (index < m_components.size())
        return m_components[index];

    return DataValueComponent();
}

void DataValue::component(uint32_t index, const DataValueComponent& val)
{
    if (index < m_components.size())
        m_components[index] = val;
}

DataValueComponent DataValue::matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row) const
{
    if (col > numCols || row > numRows)
        return DataValueComponent();

    auto index = col + row * numCols; // matrix is ALWAYS row-major in the memory
    return component(index);
}

void DataValue::matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row, const DataValueComponent& val)
{
    if (col > numCols || row > numRows)
        return;

    auto index = col + row * numCols; // matrix is ALWAYS row-major in the memory
    if (index >= m_components.size())
        return;
            
    m_components[index] = val;
}

DataValue DataValue::Merge(const DataValue& a, const DataValue& b)
{
    auto maxComponents = std::max<uint32_t>(a.m_components.size(), b.m_components.size());

    DataValue ret;
    ret.m_components.resize(maxComponents);

    for (uint32_t i = 0; i < maxComponents; ++i)
    {
        DataValueComponent compA, compB;

        if (i < a.m_components.size())
            compA = a.m_components[i];

        if (i < b.m_components.size())
            compB = b.m_components[i];

        ret.m_components[i] = DataValueComponent::Merge(compA, compB);
    }

    return ret;
}

bool DataValue::operator==(const DataValue& other) const
{
    return Compare(*this, other);
}

bool DataValue::operator!=(const DataValue& other) const
{
    return !Compare(*this, other);
}

bool DataValue::Compare(const DataValue& a, const DataValue& b)
{
    if (a.m_components.size() != b.m_components.size())
        return false;

    for (uint32_t i = 0; i < a.m_components.size(); ++i)
    {
        if (!DataValueComponent::Compare(a.m_components[i], b.m_components[i]))
            return false;
    }

    return true;
}

void DataValue::calcCRC(CRC64& outCRC) const
{
    outCRC << m_components.size();
    for (auto& val : m_components)
        val.calcCRC(outCRC);
}

namespace helper
{
    static void DumpScalars(StringBuilder& builder, DataType type, const DataValueComponent* components, const DataValueComponent* lastValidComponent)
    {
        if (!type.arrayCounts().empty())
        {
            auto numElems = type.arrayCounts().outermostCount();
            auto subType = GetArrayInnerType(type);

            builder.appendf("<array count=\"{}\">", numElems);

            auto subStep = subType.computeScalarComponentCount();
            for (uint32_t i = 0; i < numElems; ++i)
            {
                DumpScalars(builder, subType, components, lastValidComponent);
                components += subStep;
            }

            builder.append("</array>");
        }
        else if (type.isComposite())
        {
            builder.appendf("<struct name=\"{}\">", type.composite().name());

            auto& members = type.composite().members();
            for (auto& member : members)
            {
                builder.appendf("<member name=\"{}\">", member.name);
                DumpScalars(builder, member.type, components + member.firstComponent, lastValidComponent);
                builder.append("</member>");
            }

            builder.append("</struct>");
        }
        else if (type.baseType() == BaseType::Boolean || type.baseType() == BaseType::Float || type.baseType() == BaseType::Int || type.baseType() == BaseType::Uint || type.baseType() == BaseType::Name)
        {
            if (components < lastValidComponent)
            {
                builder.appendf("<value type=\"{}\">{}</value>", type, *components);
            }
            else
            {
                builder.append("<invalid/>");
            }

        }
        else if (type.baseType() == BaseType::Resource)
        {
            builder.append("<resource/>");
        }
        else
        {
            builder.append("<invalid/>");
        }
    }
}

void DataValue::print(IFormatStream& builder) const
{
    if (m_components.empty())
    {
        builder.append("empty");
    }
    else if (m_components.size() == 1)
    {
        builder << m_components[0];
    }
    else
    {
        builder.appendf("[{}]", m_components.size());
        builder.append("{");

        for (uint32_t k = 0; k < m_components.size(); ++k)
        {
            if (k) builder << ", ";
            builder << m_components[k];
        }

        builder.append("}");
    }
}

//---

DataValueRef::DataValueRef()
    : m_firstComponent(nullptr)
    , m_numComponents(0)
{}

DataValueRef::DataValueRef(const DataType& type)
    : m_type(type)
    , m_firstComponent(nullptr)
    , m_numComponents(type.computeScalarComponentCount())
{}

DataValueRef::DataValueRef(const DataType& type, DataValueComponent* firstComponent, uint32_t numComponents)
    : m_type(type)
    , m_firstComponent(firstComponent)
    , m_numComponents(numComponents)
{
    ASSERT(m_numComponents == type.computeScalarComponentCount());
}

DataValueRef::DataValueRef(const DataType& type, DataValue* value)
    : m_type(type)
{
    m_firstComponent = value->data();
    m_numComponents = value->size();
}

bool DataValueRef::isWholeValueDefined() const
{
    if (m_numComponents == 0)
        return false;

    for (uint32_t i = 0; i < m_numComponents; ++i)
        if (!m_firstComponent[i].isDefined())
            return false;

    return true;
}

DataValueComponent* DataValueRef::component(const uint32_t index /*= 0*/) const
{
    if (index >= m_numComponents)
        return nullptr;
    return m_firstComponent + index;
}

DataValueRef DataValueRef::arrayElement(uint32_t index) const
{
    // we are not an array
    if (!m_type.isArray())
        return DataValueRef();

    // get the inner array size
    auto& arrayCounts = m_type.arrayCounts();
    auto innerArrayType = m_type.applyArrayCounts(arrayCounts.innerCounts());

    // validate the array index
    if (index >= arrayCounts.outermostCount())
        return DataValueRef(innerArrayType);

    // compute the element placement
    auto elementSize = innerArrayType.computeScalarComponentCount();
    auto placementIndex = elementSize * index;
    return DataValueRef(innerArrayType, m_firstComponent + placementIndex, elementSize);
}

DataValueRef DataValueRef::compositeElement(const StringID name) const
{
    // we are not a structure
    if (!m_type.isComposite())
        return DataValueRef();

    // find the structure element
    auto& compositeType = m_type.composite();
    auto memberIndex = compositeType.memberIndex(name);
    if (memberIndex == -1)
        return DataValueRef();

    // compute the offset to member
    uint32_t offsetToMember = 0;
    for (uint32_t i=0; i<(uint32_t)memberIndex; ++i)
    {
        auto memberType = compositeType.memberType(i);
        offsetToMember += memberType.computeScalarComponentCount();
    }

    // create member reference
    auto memberType = compositeType.memberType(memberIndex);
    return DataValueRef(memberType, m_firstComponent + offsetToMember, memberType.computeScalarComponentCount());
}

void DataValueRef::print(IFormatStream& f) const
{
    if (m_firstComponent == nullptr)
    {
        f.append("nullptr");
    }
    else
    {
        f << "ref ";

        if (m_numComponents == 1)
        {
            f << m_firstComponent[0];
        }
        else
        {
            f.appendf("[{}]", m_numComponents);
            f.append("{");

            for (uint32_t k = 0; k < m_numComponents; ++k)
            {
                if (k) f << ", ";
                f << m_firstComponent[k];
            }

            f.append("}");
        }
    }
}

bool DataValueRef::readValue(DataValue& outValue) const
{
    // invalid value
    if (!valid())
        return false;

    // create output
    outValue = DataValue(m_type);

    // setup values
    for (uint32_t i=0; i<m_numComponents; ++i)
        outValue.component(i, m_firstComponent[i]);
    return true;
}

bool DataValueRef::writeValue(const DataValue& newValue) const
{
    // invalid value
    if (!valid())
        return false;

    // write value
    for (uint32_t i=0; i<m_numComponents; ++i)
        m_firstComponent[i] = newValue.component(i);

    return true;
}

bool DataValueRef::writeValueWithMask(const DataValue& newValue, const ComponentMask& mask) const
{
    // invalid value
    if (!valid())
        return false;

    InplaceArray<uint8_t, 4> writeList;
    mask.componentsToWrite(writeList);

    uint32_t readIndex = 0;
    for (auto& index : writeList)
    {
        if (index >= m_numComponents)
        {
            TRACE_ERROR("Invalid mask {}, max com: {}", index, m_numComponents);
            return false;
        }

        m_firstComponent[index] = newValue.component(readIndex);
        readIndex += 1;
    }

    return true;
}

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
