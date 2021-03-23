/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\color #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_STRUCT(Color);
    RTTI_BIND_NATIVE_COMPARE(Color);
    RTTI_PROPERTY(r).editable().range(0, 255).widgetSlider();
    RTTI_PROPERTY(g).editable().range(0, 255).widgetSlider();
    RTTI_PROPERTY(b).editable().range(0, 255).widgetSlider();
    RTTI_PROPERTY(a).editable().range(0, 255).widgetSlider();
RTTI_END_TYPE();

//---

Color::Color(StringView txt)
    : r(255), g(255), b(255), a(255)
{
    Parse(txt, *this);
}

//--

Color Color::operator*(float scale) const
{
    return FromVectorLinear(toVectorLinear() * scale);
}

Color& Color::operator*=(float scale)
{
    *this = FromVectorLinear(toVectorLinear() * scale);
    return *this;
}

Color Color::replaceAlpha(uint8_t newAlpha) const
{
    return Color(r, g, b, newAlpha);
}

Color Color::scaleColor(float scale) const
{
    Color ret(*this);
    ret.r = FloatTo255(FloatFrom255(r) * scale);
    ret.g = FloatTo255(FloatFrom255(g) * scale);
    ret.b = FloatTo255(FloatFrom255(b) * scale);
    return ret;
}

Color Color::scaleAlpha(float scale) const
{
    Color ret(*this);
    ret.a = FloatTo255(FloatFrom255(a) * scale);
    return ret;
}

// https://stackoverflow.com/questions/1102692/how-to-alpha-blend-rgba-unsigned-byte-color-fast
Color Lerp256(const Color &a, const Color &b, uint32_t alpha) // alpha is from 0 to 256
{
    uint32_t invA = 0x100 - alpha;
    uint32_t rb1 = (invA * (a.toNative() & 0xFF00FF)) >> 8;
    uint32_t rb2 = (alpha * (b.toNative() & 0xFF00FF)) >> 8;
    uint32_t g1 = (invA * (a.toNative() & 0x00FF00)) >> 8;
    uint32_t g2 = (alpha * (b.toNative() & 0x00FF00)) >> 8;
    return ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00) + 0xFF000000;
}

Vector4 Color::toVectorLinear() const
{
    Vector4 ret;

#ifdef PLATFORM_SSE2
    static const __m128 alphaOne = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    static const __m128 scalar = _mm_set_ps(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f);
    __m128i xmm0 = _mm_set1_epi32(*(const uint32_t*)this);
    xmm0 = _mm_unpacklo_epi8(xmm0, _mm_setzero_si128()); // unpack to 16 bits
    xmm0 = _mm_unpacklo_epi16(xmm0, _mm_setzero_si128());  // unpack to 32 bits
    __m128 xmm1 = _mm_cvtepi32_ps(xmm0); // convert to floats
    xmm1 = _mm_mul_ps(xmm1, scalar); // convert to 0-1 range
    //xmm1 = _mm_and_ps(xmm1, _mm_castsi128_ps(packMask)); // zero out invalid entries
    _mm_store_ps((float*)&ret, xmm1);
    return ret;
#else
    auto div = 1.0f / 255.0f;
    ret.x = r * div;
    ret.y = g * div;
    ret.z = b * div;
    ret.w = a * div;
    return ret;
#endif
}

Color Color::FromVectorLinear(const Vector4 &other)
{
    struct
    {
        Color ret;
        uint8_t padding[12];
    } data;
#ifdef PLATFORM_SSE2
    static const __m128i packMask = _mm_setr_epi8(-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    static const __m128 scalar = _mm_set_ps(255.0f, 255.0f, 255.0f, 255.0f);
    __m128 c = _mm_mul_ps(_mm_load_ps((const float*)&other), scalar);
    __m128i ci = _mm_cvtps_epi32(c);
    __m128i packedW = _mm_packs_epi32(ci, ci); // to 16-bit signed with saturation
    __m128i packedB = _mm_packus_epi16(packedW, packedW); // to 8-bit unsigned with saturation
    _mm_maskmoveu_si128(packedB, packMask, (char*)&data);
#else
    data.ret.r = FloatTo255(other.x);
    data.ret.g = FloatTo255(other.y);
    data.ret.b = FloatTo255(other.z);
    data.ret.a = FloatTo255(other.w);
#endif
    return data.ret;
}

// https://www.nayuki.io/res/srgb-transform-library/srgb-transform.c
const float SRGB_8BIT_TO_LINEAR_FLOAT[1 << 8] = {
        0.0f, 3.03527e-4f, 6.07054e-4f, 9.10581e-4f,
        0.001214108f, 0.001517635f, 0.001821162f, 0.0021246888f,
        0.002428216f, 0.002731743f, 0.00303527f, 0.0033465358f,
        0.0036765074f, 0.004024717f, 0.004391442f, 0.0047769537f,
        0.005181517f, 0.005605392f, 0.0060488335f, 0.006512091f,
        0.0069954107f, 0.007499032f, 0.008023193f, 0.008568126f,
        0.009134059f, 0.009721218f, 0.010329823f, 0.010960095f,
        0.011612245f, 0.012286489f, 0.0129830325f, 0.013702083f,
        0.014443845f, 0.015208516f, 0.015996294f, 0.016807377f,
        0.017641956f, 0.018500222f, 0.019382363f, 0.020288564f,
        0.021219011f, 0.022173885f, 0.023153368f, 0.024157634f,
        0.025186861f, 0.026241222f, 0.027320893f, 0.02842604f,
        0.029556835f, 0.030713445f, 0.031896032f, 0.033104766f,
        0.034339808f, 0.035601314f, 0.036889452f, 0.038204372f,
        0.039546236f, 0.0409152f, 0.04231141f, 0.04373503f,
        0.045186203f, 0.046665087f, 0.048171826f, 0.049706567f,
        0.051269464f, 0.05286065f, 0.05448028f, 0.056128494f,
        0.057805438f, 0.059511244f, 0.06124606f, 0.06301002f,
        0.06480327f, 0.066625945f, 0.068478175f, 0.0703601f,
        0.07227185f, 0.07421357f, 0.07618539f, 0.07818743f,
        0.08021983f, 0.082282715f, 0.084376216f, 0.086500466f,
        0.08865559f, 0.09084172f, 0.093058966f, 0.09530747f,
        0.097587354f, 0.09989873f, 0.10224174f, 0.10461649f,
        0.107023105f, 0.10946172f, 0.111932434f, 0.11443538f,
        0.11697067f, 0.119538434f, 0.122138776f, 0.12477182f,
        0.12743768f, 0.13013647f, 0.13286832f, 0.13563333f,
        0.13843162f, 0.14126329f, 0.14412847f, 0.14702727f,
        0.14995979f, 0.15292616f, 0.15592647f, 0.15896083f,
        0.16202939f, 0.1651322f, 0.1682694f, 0.17144111f,
        0.1746474f, 0.17788842f, 0.18116425f, 0.18447499f,
        0.18782078f, 0.19120169f, 0.19461784f, 0.19806932f,
        0.20155625f, 0.20507874f, 0.20863687f, 0.21223076f,
        0.21586053f, 0.21952623f, 0.22322798f, 0.2269659f,
        0.23074007f, 0.23455061f, 0.2383976f, 0.24228115f,
        0.24620135f, 0.2501583f, 0.25415212f, 0.25818288f,
        0.2622507f, 0.26635563f, 0.27049783f, 0.27467734f,
        0.2788943f, 0.28314877f, 0.28744087f, 0.29177067f,
        0.2961383f, 0.3005438f, 0.30498734f, 0.30946895f,
        0.31398875f, 0.3185468f, 0.32314324f, 0.32777813f,
        0.33245155f, 0.33716366f, 0.34191445f, 0.3467041f,
        0.35153264f, 0.35640016f, 0.36130682f, 0.36625263f,
        0.3712377f, 0.37626216f, 0.38132605f, 0.38642946f,
        0.3915725f, 0.39675525f, 0.4019778f, 0.40724024f,
        0.41254264f, 0.4178851f, 0.4232677f, 0.42869052f,
        0.43415368f, 0.4396572f, 0.44520122f, 0.45078582f,
        0.45641103f, 0.46207702f, 0.4677838f, 0.4735315f,
        0.4793202f, 0.48514995f, 0.4910209f, 0.496933f,
        0.5028865f, 0.50888133f, 0.5149177f, 0.5209956f,
        0.52711517f, 0.53327644f, 0.5394795f, 0.5457245f,
        0.55201143f, 0.55834043f, 0.5647115f, 0.57112485f,
        0.57758045f, 0.58407843f, 0.59061885f, 0.5972018f,
        0.60382736f, 0.61049557f, 0.6172066f, 0.62396044f,
        0.63075715f, 0.6375969f, 0.6444797f, 0.65140563f,
        0.65837485f, 0.66538733f, 0.67244315f, 0.6795425f,
        0.6866853f, 0.6938718f, 0.7011019f, 0.7083758f,
        0.71569353f, 0.7230551f, 0.73046076f, 0.73791045f,
        0.74540424f, 0.7529422f, 0.7605245f, 0.76815116f,
        0.7758222f, 0.7835378f, 0.791298f, 0.7991027f,
        0.8069523f, 0.8148466f, 0.82278574f, 0.8307699f,
        0.838799f, 0.8468732f, 0.8549926f, 0.8631572f,
        0.8713671f, 0.8796224f, 0.8879231f, 0.8962694f,
        0.9046612f, 0.91309863f, 0.92158186f, 0.9301109f,
        0.9386857f, 0.9473065f, 0.9559733f, 0.9646863f,
        0.9734453f, 0.9822506f, 0.9911021f, 1.0f,
};

Vector4 Color::toVectorSRGB() const
{
    // heh, still faster than computing... in 2019.. lol
    return Vector4(SRGB_8BIT_TO_LINEAR_FLOAT[r], SRGB_8BIT_TO_LINEAR_FLOAT[g], SRGB_8BIT_TO_LINEAR_FLOAT[b], SRGB_8BIT_TO_LINEAR_FLOAT[a]);
}

StringBuf Color::toHexString(bool withAlpha /*= false*/) const
{
    if (withAlpha)
        return TempString("#{}{}{}{}", Hex(r), Hex(g), Hex(b), Hex(a));
    else
        return TempString("#{}{}{}", Hex(r), Hex(g), Hex(b));
}

float Color::luminanceSRGB() const
{
    return Vector3(0.299f, 0.587f, 0.114f) | toVectorSRGB().xyz();
}

Color Color::invertedSRGB() const
{
    auto values = toVectorSRGB();
    values.x = 1.0f - values.x;
    values.y = 1.0f - values.y;
    values.z = 1.0f - values.z;
    return Color::FromVectorSRGBExact(values);
}

void Color::print(IFormatStream& f) const
{
    f << "#" << Hex(r) << Hex(g) << Hex(b);
    if (a != 255)
        f << Hex(a);
}

static float LinearToSRGB(float x)
{
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.0031308f)
        return x * 12.92f;
    else
        return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

Color Color::FromVectorSRGBFast(const Vector4 &other)
{
    Color ret;
    ret.r = FloatTo255(LinearToSRGB(other.x));
    ret.g = FloatTo255(LinearToSRGB(other.y));
    ret.b = FloatTo255(LinearToSRGB(other.z));
    ret.a = FloatTo255(LinearToSRGB(other.w));
    return true;
}

uint8_t LinearToSRGB8bit(float x)
{
    if (x <= 0.0f)
        return 0;

    if (x >= 1.0f)
        return 255;

    int y = 0;
    for (int i = 128; i != 0; i >>= 1) {
        if (SRGB_8BIT_TO_LINEAR_FLOAT[y + i] <= x)
            y += i;
    }
    if (x - SRGB_8BIT_TO_LINEAR_FLOAT[y] <= SRGB_8BIT_TO_LINEAR_FLOAT[y + 1] - x)
        return y;
    else
        return y + 1;
}

Color Color::FromVectorSRGBExact(const Vector4 &other)
{
    Color ret;
    ret.r = LinearToSRGB8bit(other.x);
    ret.g = LinearToSRGB8bit(other.y);
    ret.b = LinearToSRGB8bit(other.z);
    ret.a = LinearToSRGB8bit(other.w);
    return ret;
}

///---

bool Color::Parse(StringView txt, Color& outColor)
{
    StringParser p(txt);

    p.parseKeyword("#");

    uint32_t length = 0;
    uint64_t value = 0;
    if (p.parseHex(value, 0, &length))
    {
        if (length == 3)
        {
            uint8_t r = (value >> 8) & 0xF;
            uint8_t g = (value >> 4) & 0xF;
            uint8_t b = (value >> 0) & 0xF;

            r = r | (r << 4);
            g = g | (g << 4);
            b = b | (b << 4);

            outColor = Color(r, g, b, 255);
            return true;
        }
        else if (length == 4)
        {
            uint8_t r = (value >> 12) & 0xF;
            uint8_t g = (value >> 8) & 0xF;
            uint8_t b = (value >> 4) & 0xF;
            uint8_t a = (value >> 0) & 0xF;

            r = r | (r << 4);
            g = g | (g << 4);
            b = b | (b << 4);
            a = a | (a << 4);

            outColor = Color(r, g, b, a);
            return true;
        }
        else if (length == 6)
        {
            uint8_t r = (value >> 16) & 0xFF;
            uint8_t g = (value >> 8) & 0xFF;
            uint8_t b = (value >> 0) & 0xFF;

            outColor = Color(r, g, b, 255);
            return true;
        }
        else if (length == 8)
        {
            uint8_t r = (value >> 24) & 0xFF;
            uint8_t g = (value >> 16) & 0xFF;
            uint8_t b = (value >> 8) & 0xFF;
            uint8_t a = (value >> 0) & 0xFF;

            outColor = Color(r, g, b, a);
            return true;
        }
    }

    return false;
}

///---

const Color::ColorVal Color::NONE = 0x00FFFFFF; // alpha zero
const Color::ColorVal Color::MAROON = 0xFF800000;
const Color::ColorVal Color::DARKRED = 0xFF8B0000;
const Color::ColorVal Color::RED = 0xFFFF0000;
const Color::ColorVal Color::LIGHTPINK = 0xFFFFB6C1;
const Color::ColorVal Color::CRIMSON = 0xFFDC143C;
const Color::ColorVal Color::PALEVIOLETRED = 0xFFDB7093;
const Color::ColorVal Color::HOTPINK = 0xFFFF69B4;
const Color::ColorVal Color::DEEPPINK = 0xFFFF1493;
const Color::ColorVal Color::MEDIUMVIOLETRED = 0xFFC71585;
const Color::ColorVal Color::PURPLE = 0xFF800080;
const Color::ColorVal Color::DARKMAGENTA = 0xFF8B008B;
const Color::ColorVal Color::ORCHID = 0xFFDA70D6;
const Color::ColorVal Color::THISTLE = 0xFFD8BFD8;
const Color::ColorVal Color::PLUM = 0xFFDDA0DD;
const Color::ColorVal Color::VIOLET = 0xFFEE82EE;
const Color::ColorVal Color::FUCHSIA = 0xFFFF00FF;
const Color::ColorVal Color::MAGENTA = 0xFFFF00FF;
const Color::ColorVal Color::MEDIUMORCHID = 0xFFBA55D3;
const Color::ColorVal Color::DARKVIOLET = 0xFF9400D3;
const Color::ColorVal Color::DARKORCHID = 0xFF9932CC;
const Color::ColorVal Color::BLUEVIOLET = 0xFF8A2BE2;
const Color::ColorVal Color::INDIGO = 0xFF4B0082;
const Color::ColorVal Color::MEDIUMPURPLE = 0xFF9370DB;
const Color::ColorVal Color::SLATEBLUE = 0xFF6A5ACD;
const Color::ColorVal Color::MEDIUMSLATEBLUE = 0xFF7B68EE;
const Color::ColorVal Color::DARKBLUE = 0xFF00008B;
const Color::ColorVal Color::MEDIUMBLUE = 0xFF0000CD;
const Color::ColorVal Color::BLUE = 0xFF0000FF;
const Color::ColorVal Color::NAVY = 0xFF000080;
const Color::ColorVal Color::MIDNIGHTBLUE = 0xFF191970;
const Color::ColorVal Color::DARKSLATEBLUE = 0xFF483D8B;
const Color::ColorVal Color::ROYALBLUE = 0xFF4169E1;
const Color::ColorVal Color::CORNFLOWERBLUE = 0xFF6495ED;
const Color::ColorVal Color::LIGHTSTEELBLUE = 0xFFB0C4DE;
const Color::ColorVal Color::ALICEBLUE = 0xFFF0F8FF;
const Color::ColorVal Color::GHOSTWHITE = 0xFFF8F8FF;
const Color::ColorVal Color::LAVENDER = 0xFFE6E6FA;
const Color::ColorVal Color::DODGERBLUE = 0xFF1E90FF;
const Color::ColorVal Color::STEELBLUE = 0xFF4682B4;
const Color::ColorVal Color::DEEPSKYBLUE = 0xFF00BFFF;
const Color::ColorVal Color::SLATEGRAY = 0xFF708090;
const Color::ColorVal Color::LIGHTSLATEGRAY = 0xFF778899;
const Color::ColorVal Color::LIGHTSKYBLUE = 0xFF87CEFA;
const Color::ColorVal Color::SKYBLUE = 0xFF87CEEB;
const Color::ColorVal Color::LIGHTBLUE = 0xFFADD8E6;
const Color::ColorVal Color::TEAL = 0xFF008080;
const Color::ColorVal Color::DARKCYAN = 0xFF008B8B;
const Color::ColorVal Color::DARKTURQUOISE = 0xFF00CED1;
const Color::ColorVal Color::CYAN = 0xFF00FFFF;
const Color::ColorVal Color::MEDIUMTURQUOISE = 0xFF48D1CC;
const Color::ColorVal Color::CADETBLUE = 0xFF5F9EA0;
const Color::ColorVal Color::PALETURQUOISE = 0xFFAFEEEE;
const Color::ColorVal Color::LIGHTCYAN = 0xFFE0FFFF;
const Color::ColorVal Color::AZURE = 0xFFF0FFFF;
const Color::ColorVal Color::LIGHTSEAGREEN = 0xFF20B2AA;
const Color::ColorVal Color::TURQUOISE = 0xFF40E0D0;
const Color::ColorVal Color::POWDERBLUE = 0xFFB0E0E6;
const Color::ColorVal Color::DARKSLATEGRAY = 0xFF2F4F4F;
const Color::ColorVal Color::AQUAMARINE = 0xFF7FFFD4;
const Color::ColorVal Color::MEDIUMSPRINGGREEN = 0xFF00FA9A;
const Color::ColorVal Color::MEDIUMAQUAMARINE = 0xFF66CDAA;
const Color::ColorVal Color::SPRINGGREEN = 0xFF00FF7F;
const Color::ColorVal Color::MEDIUMSEAGREEN = 0xFF3CB371;
const Color::ColorVal Color::SEAGREEN = 0xFF2E8B57;
const Color::ColorVal Color::LIMEGREEN = 0xFF32CD32;
const Color::ColorVal Color::DARKGREEN = 0xFF006400;
const Color::ColorVal Color::GREEN = 0xFF00FF00;
const Color::ColorVal Color::LIME = 0xFF008000;
const Color::ColorVal Color::FORESTGREEN = 0xFF228B22;
const Color::ColorVal Color::DARKSEAGREEN = 0xFF8FBC8F;
const Color::ColorVal Color::LIGHTGREEN = 0xFF90EE90;
const Color::ColorVal Color::PALEGREEN = 0xFF98FB98;
const Color::ColorVal Color::MINTCREAM = 0xFFF5FFFA;
const Color::ColorVal Color::HONEYDEW = 0xFFF0FFF0;
const Color::ColorVal Color::CHARTREUSE = 0xFF7FFF00;
const Color::ColorVal Color::LAWNGREEN = 0xFF7CFC00;
const Color::ColorVal Color::OLIVEDRAB = 0xFF6B8E23;
const Color::ColorVal Color::DARKOLIVEGREEN = 0xFF556B2F;
const Color::ColorVal Color::YELLOWGREEN = 0xFF9ACD32;
const Color::ColorVal Color::GREENYELLOW = 0xFFADFF2F;
const Color::ColorVal Color::BEIGE = 0xFFF5F5DC;
const Color::ColorVal Color::LINEN = 0xFFFAF0E6;
const Color::ColorVal Color::LIGHTGOLDENRODYELLOW = 0xFFFAFAD2;
const Color::ColorVal Color::OLIVE = 0xFF808000;
const Color::ColorVal Color::YELLOW = 0xFFFFFF00;
const Color::ColorVal Color::LIGHTYELLOW = 0xFFFFFFE0;
const Color::ColorVal Color::IVORY = 0xFFFFFFF0;
const Color::ColorVal Color::DARKKHAKI = 0xFFBDB76B;
const Color::ColorVal Color::KHAKI = 0xFFF0E68C;
const Color::ColorVal Color::PALEGOLDENROD = 0xFFEEE8AA;
const Color::ColorVal Color::WHEAT = 0xFFF5DEB3;
const Color::ColorVal Color::GOLD = 0xFFFFD700;
const Color::ColorVal Color::LEMONCHIFFON = 0xFFFFFACD;
const Color::ColorVal Color::PAPAYAWHIP = 0xFFFFEFD5;
const Color::ColorVal Color::DARKGOLDENROD = 0xFFB8860B;
const Color::ColorVal Color::GOLDENROD = 0xFFDAA520;
const Color::ColorVal Color::ANTIQUEWHITE = 0xFFFAEBD7;
const Color::ColorVal Color::CORNSILK = 0xFFFFF8DC;
const Color::ColorVal Color::OLDLACE = 0xFFFDF5E6;
const Color::ColorVal Color::MOCCASIN = 0xFFFFE4B5;
const Color::ColorVal Color::NAVAJOWHITE = 0xFFFFDEAD;
const Color::ColorVal Color::ORANGE = 0xFFFFA500;
const Color::ColorVal Color::BISQUE = 0xFFFFE4C4;
const Color::ColorVal Color::TAN = 0xFFD2B48C;
const Color::ColorVal Color::DARKORANGE = 0xFFFF8C00;
const Color::ColorVal Color::BURLYWOOD = 0xFFDEB887;
const Color::ColorVal Color::SADDLEBROWN = 0xFF8B4513;
const Color::ColorVal Color::SANDYBROWN = 0xFFF4A460;
const Color::ColorVal Color::BLANCHEDALMOND = 0xFFFFEBCD;
const Color::ColorVal Color::LAVENDERBLUSH = 0xFFFFF0F5;
const Color::ColorVal Color::SEASHELL = 0xFFFFF5EE;
const Color::ColorVal Color::FLORALWHITE = 0xFFFFFAF0;
const Color::ColorVal Color::SNOW = 0xFFFFFAFA;
const Color::ColorVal Color::PERU = 0xFFCD853F;
const Color::ColorVal Color::PEACHPUFF = 0xFFFFDAB9;
const Color::ColorVal Color::CHOCOLATE = 0xFFD2691E;
const Color::ColorVal Color::SIENNA = 0xFFA0522D;
const Color::ColorVal Color::LIGHTSALMON = 0xFFFFA07A;
const Color::ColorVal Color::CORAL = 0xFFFF7F50;
const Color::ColorVal Color::DARKSALMON = 0xFFE9967A;
const Color::ColorVal Color::MISTYROSE = 0xFFFFE4E1;
const Color::ColorVal Color::ORANGERED = 0xFFFF4500;
const Color::ColorVal Color::SALMON = 0xFFFA8072;
const Color::ColorVal Color::TOMATO = 0xFFFF6347;
const Color::ColorVal Color::ROSYBROWN = 0xFFBC8F8F;
const Color::ColorVal Color::PINK = 0xFFFFC0CB;
const Color::ColorVal Color::INDIANRED = 0xFFCD5C5C;
const Color::ColorVal Color::LIGHTCORAL = 0xFFF08080;
const Color::ColorVal Color::BROWN = 0xFFA52A2A;
const Color::ColorVal Color::FIREBRICK = 0xFFB22222;
const Color::ColorVal Color::BLACK = 0xFF000000;
const Color::ColorVal Color::DIMGRAY = 0xFF696969;
const Color::ColorVal Color::GRAY = 0xFF808080;
const Color::ColorVal Color::DARKGRAY = 0xFFA9A9A9;
const Color::ColorVal Color::SILVER = 0xFFC0C0C0;
const Color::ColorVal Color::LIGHTGREY = 0xFFD3D3D3;
const Color::ColorVal Color::GAINSBORO = 0xFFDCDCDC;
const Color::ColorVal Color::WHITESMOKE = 0xFFF5F5F5;
const Color::ColorVal Color::WHITE = 0xFFFFFFFF;
const Color::ColorVal Color::GREY       = 0xff888888;
const Color::ColorVal Color::GREY25 = 0xff404040;
const Color::ColorVal Color::GREY50 = 0xff808080;
const Color::ColorVal Color::GREY75 = 0xffc0c0c0;

const Color::ColorVal Color::TROVE0 = Color("#51574A").toABGR();
const Color::ColorVal Color::TROVE1 = Color("#447C69").toABGR();
const Color::ColorVal Color::TROVE2 = Color("#74C493").toABGR();
const Color::ColorVal Color::TROVE3 = Color("#8E8C6D").toABGR();
const Color::ColorVal Color::TROVE4 = Color("#E4BF80").toABGR();
const Color::ColorVal Color::TROVE5 = Color("#E9D78E").toABGR();
const Color::ColorVal Color::TROVE6 = Color("#E2975D").toABGR();
const Color::ColorVal Color::TROVE7 = Color("#F19670").toABGR();
const Color::ColorVal Color::TROVE8 = Color("#E16552").toABGR();
const Color::ColorVal Color::TROVE9 = Color("#C94A53").toABGR();
const Color::ColorVal Color::TROVE10 = Color("#BE5168").toABGR();
const Color::ColorVal Color::TROVE11 = Color("#A34974").toABGR();
const Color::ColorVal Color::TROVE12 = Color("#993767").toABGR();
const Color::ColorVal Color::TROVE13 = Color("#65387D").toABGR();
const Color::ColorVal Color::TROVE14 = Color("#4E2472").toABGR();
const Color::ColorVal Color::TROVE15 = Color("#9163B6").toABGR();
const Color::ColorVal Color::TROVE16 = Color("#E279A3").toABGR();
const Color::ColorVal Color::TROVE17 = Color("#E0598B").toABGR();
const Color::ColorVal Color::TROVE18 = Color("#7C9FB0").toABGR();
const Color::ColorVal Color::TROVE19 = Color("#5698C4").toABGR();
const Color::ColorVal Color::TROVE20 = Color("#9ABF88").toABGR();

///---

END_BOOMER_NAMESPACE()
