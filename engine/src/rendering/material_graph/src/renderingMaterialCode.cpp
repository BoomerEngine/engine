/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialCode.h"
#include "renderingMaterialGraphBlock.h"
#include "renderingMaterialGraph.h"

#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphConnection.h"
#include "base/graph/include/graphSocket.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"

namespace rendering
{
    ///---

    CodeChunk::CodeChunk(const CodeChunk& other) = default;
    CodeChunk::CodeChunk(CodeChunk&& other) = default;
    CodeChunk& CodeChunk::operator=(const CodeChunk& other) = default;
    CodeChunk& CodeChunk::operator=(CodeChunk&& other) = default;

    bool CodeChunk::operator==(const CodeChunk& other) const
    {
        return m_text == other.m_text && m_constant == other.m_constant && m_type == other.m_type;
    }

    bool CodeChunk::operator!=(const CodeChunk& other) const
    {
        return !operator==(other);
    }

    static CodeChunk theEmptyChunk;

    const CodeChunk& CodeChunk::EMPTY()
    {
        return theEmptyChunk;
    }

    uint8_t CodeChunk::components() const
    {
        switch (m_type)
        {
            case CodeChunkType::Numerical1: return 1;
            case CodeChunkType::Numerical2: return 2;
            case CodeChunkType::Numerical3: return 3;
            case CodeChunkType::Numerical4: return 4;
        }
        return 0;
    }

    CodeChunk::CodeChunk(float val)
        : m_type(CodeChunkType::Numerical1)
        , m_constant(true)
        , m_text(base::TempString("{}", val))
    {        
    }

    CodeChunk::CodeChunk(uint32_t val)
        : m_type(CodeChunkType::Numerical1)
        , m_constant(true)
        , m_text(base::TempString("{}", val))
    {
    }

    CodeChunk::CodeChunk(const base::Vector2& val, CodeChunkType type_ /*= CodeChunkType::Numerical2*/)
        : m_type(type_)
        , m_constant(true)
        , m_text(base::TempString("vec2({}, {})", val.x, val.y))
    {
    }

    CodeChunk::CodeChunk(const base::Vector3& val, CodeChunkType type_ /*= CodeChunkType::Numerical3*/)
        : m_type(type_)
        , m_constant(true)
        , m_text(base::TempString("vec3({}, {}, {})", val.x, val.y, val.z))
    {
    }

    CodeChunk::CodeChunk(const base::Vector4& val, CodeChunkType type_ /*= CodeChunkType::Numerical4*/)
        : m_type(type_)
        , m_constant(true)
        , m_text(base::TempString("vec4({}, {}, {}, {})", val.x, val.y, val.z, val.w))
    {
    }

    CodeChunk::CodeChunk(CodeChunkType type_, base::StringView<char> text_, bool isConstant /*= false*/)
        : m_type(type_)
        , m_constant(isConstant)
        , m_text(text_)
    {}

    void CodeChunk::print(base::IFormatStream& f) const
    {
        f << m_text;
    }

    ///---

    static CodeChunkType TypeForComponentCount(uint8_t count)
    {
        DEBUG_CHECK_EX(count >= 1 && count <= 4, "Invalid component count");

        switch (count)
        {
            case 1: return CodeChunkType::Numerical1;
            case 2: return CodeChunkType::Numerical2;
            case 3: return CodeChunkType::Numerical3;
            case 4: return CodeChunkType::Numerical4;
        }

        return CodeChunkType::Void;
    }

    CodeChunk CodeChunk::swizzle(base::StringView<char> mask) const
    {
        const auto outCount = mask.length();
        DEBUG_CHECK_EX(outCount >= 1 && outCount <= 4, "Invalid mask length");

        uint8_t inCount = 1;
        for (const auto ch : mask)
        {
            switch (ch)
            {
                case 'x': inCount = std::max<uint8_t>(inCount, 1); break;
                case 'y': inCount = std::max<uint8_t>(inCount, 2); break;
                case 'z': inCount = std::max<uint8_t>(inCount, 3); break;
                case 'w': inCount = std::max<uint8_t>(inCount, 4); break;
            }
        }

        return swizzle(inCount, outCount, mask);
    }

    CodeChunk CodeChunk::conform(uint32_t componentCount)
    {
        const auto currentCount = components();
        if (currentCount == 0)
            return *this;

        if (currentCount == componentCount)
            return *this;

        switch (componentCount)
        {
        case 1:
            return x();

        case 2:
            if (currentCount >= 2)
                return xy();
            else
                return swizzle("xx");

        case 3:
            if (currentCount >= 3)
                return xyz();
            else if (currentCount == 2)
                return swizzle("xy0");
            else
                return swizzle("xxx");

        case 4:
            if (currentCount == 3)
                return swizzle("xyz1");
            else if (currentCount == 2)
                return swizzle("xy01");
            else if (currentCount == 1)
                return swizzle("xxxx");
        }

        return CodeChunk();
    }

    CodeChunk CodeChunk::swizzle(uint8_t inCount, uint8_t outCount, base::StringView<char> mask) const
    {
        DEBUG_CHECK_EX(inCount <= components(), "Invalid swizzle for current code chunk type");
        return CodeChunk(TypeForComponentCount(outCount), base::TempString("({}).{}", *this, mask), m_constant);
    }

    CodeChunkType CodeChunk::resolveBinaryOpType(const CodeChunk& other) const
    {
        const auto thisComps = components();
        const auto otherComps = other.components();

        DEBUG_CHECK_EX(0 != thisComps, "Non numerical type cannot participate in binary operation");
        DEBUG_CHECK_EX(0 != otherComps, "Non numerical type cannot participate in binary operation");

        if (otherComps == 1)
            return TypeForComponentCount(thisComps);
        else if (thisComps == 1)
            return TypeForComponentCount(otherComps);

        DEBUG_CHECK_EX(otherComps == thisComps, "Binary operation expects the same amount of components in the operands");
        return TypeForComponentCount(std::max<uint8_t>(thisComps, otherComps));
    }

    CodeChunk CodeChunk::binaryOp(const CodeChunk& other, base::StringView<char> op) const
    {
        const auto outType = resolveBinaryOpType(other);
        return CodeChunk(outType, base::TempString("({} {} {})", *this, op, other), m_constant && other.m_constant);
    }

    CodeChunk CodeChunk::binaryOpF(base::StringView<char> op, float value) const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Non numerical type cannot participate in binary operation");

        return CodeChunk(m_type, base::TempString("({} {} {})", *this, op, value), m_constant);
    }

    CodeChunk CodeChunk::binaryOpF(float value, base::StringView<char> op) const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Non numerical type cannot participate in binary operation");

        return CodeChunk(m_type, base::TempString("({} {} {})", value, op, *this), m_constant);
    }

    CodeChunk CodeChunk::unaryOp(base::StringView<char> op) const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Non numerical type cannot participate in unary operation");

        return CodeChunk(m_type, base::TempString("({}{})", op, *this), m_constant);
    }

    ///---

    CodeChunk CodeChunk::saturate() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("saturate({})", *this), m_constant);
    }

    CodeChunk CodeChunk::abs() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("abs({})", *this), m_constant);
    }

    CodeChunk CodeChunk::length() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");
        DEBUG_CHECK_EX(1 != thisComps, "Length of scalar makes no sense");

        return CodeChunk(CodeChunkType::Numerical1, base::TempString("length({})", *this), m_constant);
    }

    CodeChunk CodeChunk::normalize() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");
        DEBUG_CHECK_EX(1 != thisComps, "Normalization of scalar makes no sense");

        return CodeChunk(m_type, base::TempString("normalize({})", *this), m_constant);
    }

    CodeChunk CodeChunk::sqrt() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("sqrt({})", *this), m_constant);
    }

    CodeChunk CodeChunk::log() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("log({})", *this), m_constant);
    }

    CodeChunk CodeChunk::log2() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("log2({})", *this), m_constant);
    }

    CodeChunk CodeChunk::exp() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("exp({})", *this), m_constant);
    }

    CodeChunk CodeChunk::exp2() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("exp2({})", *this), m_constant);
    }

    CodeChunk CodeChunk::pow(const CodeChunk& other) const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");
        DEBUG_CHECK_EX(other.components() == 1, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("pow({}, {})", *this, other), m_constant);
    }

    CodeChunk CodeChunk::pow(float other) const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("pow({}, {})", *this, other), m_constant);
    }

    CodeChunk CodeChunk::floor() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("floor({})", *this), m_constant);
    }

    CodeChunk CodeChunk::round() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("round({})", *this), m_constant);
    }

    CodeChunk CodeChunk::ceil() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("ceil({})", *this), m_constant);
    }

    CodeChunk CodeChunk::frac() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("frac({})", *this), m_constant);
    }

    CodeChunk CodeChunk::sign() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("sign({})", *this), m_constant);
    }

    CodeChunk CodeChunk::ddx() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("ddx({})", *this), m_constant);
    }

    CodeChunk CodeChunk::ddy() const
    {
        const auto thisComps = components();
        DEBUG_CHECK_EX(0 != thisComps, "Function expects numerical argument");

        return CodeChunk(m_type, base::TempString("ddy({})", *this), m_constant);
    }

    //--

    CodeChunk CodeChunk::operator[](const CodeChunk& index) const
    {
        return CodeChunk(m_type, base::TempString("{}[{}]", *this, index), m_constant);
    }

    //--

    namespace CodeChunkOp
    {
        CodeChunk All(const CodeChunk& a)
        {
            DEBUG_CHECK_EX(a.components(), "Expected operand with components");
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("all({})", a), a.constant());
        }

        CodeChunk Any(const CodeChunk& a)
        {
            DEBUG_CHECK_EX(a.components(), "Expected operand with components");
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("any({})", a), a.constant());
        }

        CodeChunk Min(const CodeChunk& a, const CodeChunk& b)
        {
            const auto outType = a.resolveBinaryOpType(b);
            return CodeChunk(outType, base::TempString("min({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Max(const CodeChunk& a, const CodeChunk& b)
        {
            const auto outType = a.resolveBinaryOpType(b);
            return CodeChunk(outType, base::TempString("max({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Dot2(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(2 == a.components(), "Expected operand with 2 components");
            DEBUG_CHECK_EX(2 == b.components(), "Expected operand with 2 components");
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("dot({}.xy, {}.xy)", a, b), a.constant() && b.constant());
        }

        CodeChunk Dot3(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(3 == a.components(), "Expected operand with 3 components");
            DEBUG_CHECK_EX(3 == b.components(), "Expected operand with 3 components");
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("dot({}.xyz, {}.xyz)", a, b), a.constant() && b.constant());
        }

        CodeChunk Dot4(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(4 == a.components(), "Expected operand with 4 components");
            DEBUG_CHECK_EX(4 == b.components(), "Expected operand with 4 components");
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("dot({}.xyzw, {}.xyzw)", a, b), a.constant() && b.constant());
        }
        
        CodeChunk Clamp(const CodeChunk& x, const CodeChunk& a, const CodeChunk& b)
        {
            auto comp = x.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == a.components(), "Expected operand with the same number of components");
            DEBUG_CHECK_EX(comp == b.components(), "Expected operand with the same number of components");
            return CodeChunk(x.type(), base::TempString("clamp({}, {}, {})", x, a, b), x.constant() && a.constant() && b.constant());
        }

        CodeChunk Cross(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(3 == a.components(), "Expected operand with 3 components");
            DEBUG_CHECK_EX(3 == b.components(), "Expected operand with 3 components");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("cross({}.xyz, {}.xyz)", a, b), a.constant() && b.constant());
        }

        CodeChunk Lerp(const CodeChunk& x, const CodeChunk& y, const CodeChunk& s)
        {
            auto comp = x.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == y.components(), "Expected operand with the same number of components");
            DEBUG_CHECK_EX(1 == s.components(), "Expected scalar operand");
            return CodeChunk(x.type(), base::TempString("lerp({}, {}, {})", x, y, s), x.constant() && y.constant() && s.constant());
        }

        CodeChunk Sign(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("sign({})", a), a.constant());
        }

        CodeChunk Step(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("step({})", a), a.constant());
        }

        CodeChunk SmoothStep(const CodeChunk& min, const CodeChunk& max, const CodeChunk& s)
        {
            auto comp = min.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == max.components(), "Expected operand with the same number of components");
            DEBUG_CHECK_EX(comp == s.components(), "Expected operand with the same number of components");
            return CodeChunk(s.type(), base::TempString("smoothstep({}, {}, {})", min, max, s), min.constant() && max.constant() && s.constant());
        }

        CodeChunk Mad(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == b.components(), "Expected operand with the same number of components");
            DEBUG_CHECK_EX(comp == c.components(), "Expected operand with the same number of components");
            return CodeChunk(a.type(), base::TempString("mad({}, {}, {})", a, b, c), a.constant() && b.constant() && c.constant());
        }

        CodeChunk Sin(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("sin({})", a), a.constant());
        }

        CodeChunk Cos(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("cos({})", a), a.constant());
        }

        CodeChunk Tan(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("tan({})", a), a.constant());
        }

        CodeChunk ArcSin(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("asin({})", a), a.constant());
        }

        CodeChunk ArcCos(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("acos({})", a), a.constant());
        }

        CodeChunk ArcTan(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("atan({})", a), a.constant());
        }

        CodeChunk ArcTan2(const CodeChunk& a, const CodeChunk& b)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == b.components(), "Expected operand with the same number of components");
            return CodeChunk(a.type(), base::TempString("atan2({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Pow(const CodeChunk& a, const CodeChunk& b)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            DEBUG_CHECK_EX(comp == b.components(), "Expected operand with the same number of components");
            return CodeChunk(a.type(), base::TempString("pow({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Floor(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("floor({})", a), a.constant());
        }

        CodeChunk Ceil(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("ceil({})", a), a.constant());
        }

        CodeChunk Fract(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("frac({})", a), a.constant());
        }

        CodeChunk Trunc(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("trunc({})", a), a.constant());
        }

        CodeChunk Round(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 1 && comp <= 4, "Expected operand with numerical type");
            return CodeChunk(a.type(), base::TempString("round({})", a), a.constant());
        }

        CodeChunk Reflect(const CodeChunk& v, const CodeChunk& norm)
        {
            return v.xyz() - 2.0f * Dot3(v, norm) * norm.xyz();
        }

        CodeChunk Normalize(const CodeChunk& a)
        {
            auto comp = a.components();
            DEBUG_CHECK_EX(comp >= 2 && comp <= 4, "Expected vector operant");
            return CodeChunk(a.type(), base::TempString("normalize({})", a), a.constant());
        }

        CodeChunk Float2(const CodeChunk& a)
        {
            DEBUG_CHECK_EX(a.components() == 2, "Expected operand with 2 components");
            return CodeChunk(CodeChunkType::Numerical2, base::TempString("vec2({})", a), a.constant());
        }

        CodeChunk Float2(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components()) == 2, "Expected 2 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical2, base::TempString("vec2({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Float3(const CodeChunk& a)
        {
            DEBUG_CHECK_EX(a.components() == 3, "Expected operand with 3 components");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec3({})", a), a.constant());
        }

        CodeChunk Float3(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components()) == 3, "Expected 3 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec3({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Float3(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(c.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components() + c.components()) == 3, "Expected 3 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec3({}, {}, {})", a, b, c), a.constant() && b.constant() && c.constant());
        }

        CodeChunk Float4(const CodeChunk& a)
        {
            DEBUG_CHECK_EX(a.components() == 4, "Expected operand with 3 components");
            return CodeChunk(CodeChunkType::Numerical4, base::TempString("vec4({})", a), a.constant());
        }

        CodeChunk Float4(const CodeChunk& a, const CodeChunk& b)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components()) == 4, "Expected 4 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec4({}, {})", a, b), a.constant() && b.constant());
        }

        CodeChunk Float4(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(c.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components() + c.components()) == 4, "Expected 4 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec4({}, {}, {})", a, b, c), a.constant() && b.constant() && c.constant());
        }

        CodeChunk Float4(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c, const CodeChunk& d)
        {
            DEBUG_CHECK_EX(a.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(b.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(c.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX(d.components() != 0, "Expected numerical operand");
            DEBUG_CHECK_EX((a.components() + b.components() + c.components() + d.components()) == 4, "Expected 4 components total from all operands");
            return CodeChunk(CodeChunkType::Numerical3, base::TempString("vec4({}, {}, {}, {})", a, b, c, d), a.constant() && b.constant() && c.constant() && d.constant());
        }

    } // CodeChunkOp

    //--

    static MaterialVertexDataInfo GMaterialVertexDataInfoTable[(int)MaterialVertexDataType::MAX] = {
        {CodeChunkType::Numerical1, ImageFormat::R32_UINT, "uint", "ObjectID"},
        {CodeChunkType::Numerical1, ImageFormat::R32_UINT, "uint", "SubObjectID"},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "VertexPosition", true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "VertexNormal", true, true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "VertexTangent", true, true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "VertexBitangent", true, true},
        {CodeChunkType::Numerical4, ImageFormat::RGBA8_UNORM, "vec4", "VertexColor0", true},
        {CodeChunkType::Numerical4, ImageFormat::RGBA8_UNORM, "vec4", "VertexColor1", true},
        {CodeChunkType::Numerical4, ImageFormat::RGBA8_UNORM, "vec4", "VertexColor2", true},
        {CodeChunkType::Numerical4, ImageFormat::RGBA8_UNORM, "vec4", "VertexColor3", true},
        {CodeChunkType::Numerical2, ImageFormat::RG32F, "vec2", "VertexUV0", true},
        {CodeChunkType::Numerical2, ImageFormat::RG32F, "vec2", "VertexUV1", true},
        {CodeChunkType::Numerical2, ImageFormat::RG32F, "vec2", "VertexUV2", true},
        {CodeChunkType::Numerical2, ImageFormat::RG32F, "vec2", "VertexUV3", true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "WorldPosition"},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "WorldNormal", false, true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "WorldTangent", false, true},
        {CodeChunkType::Numerical3, ImageFormat::RGB32F, "vec3", "WorldBitangent", false, true},
    };

    const MaterialVertexDataInfo& GetMaterialVertexDataInfo(MaterialVertexDataType type)
    {
        DEBUG_CHECK((int)type <= (int)MaterialVertexDataType::MAX);
        return GMaterialVertexDataInfoTable[(int)type];
    }

    //--

    MaterialStageCompiler::MaterialStageCompiler(const MaterialDataLayout* dataLayout, rendering::ShaderType stage, base::StringView<char> materialPath, const MaterialCompilationSetup& context)
        : m_stage(stage)
        , m_materialPath(materialPath)
        , m_dataLayout(dataLayout)
        , m_context(context)
    {}

    const MaterialDataLayoutEntry* MaterialStageCompiler::findParamEntry(base::StringID entryName) const
    {
        for (const auto& entry : m_dataLayout->entries())
            if (entry.name == entryName)
                return &entry;

        return nullptr;
    }

    base::StringBuf MaterialStageCompiler::autoName()
    {
        return base::TempString("temp{}", m_autoNameCounter++);
    }

    /*static CodeChunkType VarTypeToCodeChunkType(base::StringView<char> type)
    {
        if (type == "float" || type == "bool" || type == "int" || type == "uint")
            return CodeChunkType::Numerical1;
        if (type == "vec2" || type == "ivec2" || type == "uvec2" || type == "bvec2")
            return CodeChunkType::Numerical2;
        if (type == "vec3" || type == "ivec3" || type == "uvec3" || type == "bvec3")
            return CodeChunkType::Numerical3;
        if (type == "vec4" || type == "ivec4" || type == "uvec4" || type == "bvec4")
            return CodeChunkType::Numerical4;
        return CodeChunkType::Void;
    }*/

    CodeChunk MaterialStageCompiler::var(const CodeChunk& value)
    {
        DEBUG_CHECK_EX(value.components() >= 1 && value.components() <= 4, "Invalid data to put into local variable");

        switch (value.type())
        {
            case CodeChunkType::Numerical1: append("float "); break;
            case CodeChunkType::Numerical2: append("vec2 "); break;
            case CodeChunkType::Numerical3: append("vec3 "); break;
            case CodeChunkType::Numerical4: append("vec4 "); break;
        }

        const auto varName = autoName();
        appendf("{} = {};\n", varName, value);

        return CodeChunk(value.type(), varName);
    }

    bool MaterialStageCompiler::resolveOutputBlock(const MaterialGraphBlock* sourceBlock, base::StringID socketName, const MaterialGraphBlock*& outOutputBlock, base::StringID& outOutputName, base::StringID& outOutputSwizzle) const
    {
        if (sourceBlock)
        {
            if (auto inputSocket = sourceBlock->findSocket(socketName))
            {
                const auto& connections = inputSocket->connections();
                DEBUG_CHECK_EX(connections.size() == 0 || connections.size() == 1, "Strange number of connections for input socket");

                if (connections.size() == 1)
                {
                    if (auto outputSocket = connections[0]->otherSocket(inputSocket))
                    {
                        outOutputBlock = base::rtti_cast<MaterialGraphBlock>(outputSocket->block());
                        if (outOutputBlock)
                        {
                            outOutputSwizzle = outputSocket->info().m_swizzle;
                            outOutputName = outOutputSwizzle ? base::StringID() : outputSocket->name();
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    CodeChunk MaterialStageCompiler::evalInput(const MaterialGraphBlock* sourceBlock, base::StringID socketName, const CodeChunk& defaultValue)
    {
        const MaterialGraphBlock* outputBlock = nullptr;
        base::StringID outputName, outputSwizzle;

        if (resolveOutputBlock(sourceBlock, socketName, outputBlock, outputName, outputSwizzle))
        {
            MaterialOutputBlockSocketKey key;
            key.block = outputBlock;
            key.output = outputName;

            // already cached ?
            if (const auto* code = m_compiledBlockOutputs.find(key))
                return (outputSwizzle.empty() || outputSwizzle == "-") ? *code : code->swizzle(outputSwizzle.view());

            // evaluate block output
            auto blockOutput = outputBlock->compile(*this, outputName);
            if (!blockOutput.empty())
            {
                if (!blockOutput.constant())
                    blockOutput = var(blockOutput);

                m_compiledBlockOutputs[key] = blockOutput;
                return (outputSwizzle.empty() || outputSwizzle == "-") ? blockOutput : blockOutput.swizzle(outputSwizzle.view());
            }
            else
            {
                //TRACE_ERROR("{}: error: failed to compile output for block '{}'", compiler()->path(), outputBlock->caption());
            }
        }

        return defaultValue;
    }

    void MaterialStageCompiler::includeHeader(base::StringView<char> name)
    {
        if (!name.empty())
            m_includes.pushBackUnique(base::StringBuf(name));
    }

    //--

} // rendering
 