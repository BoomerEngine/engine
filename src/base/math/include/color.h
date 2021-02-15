/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\color #]
***/

#pragma once

namespace base
{
    //--

    /// Color
    class BASE_MATH_API Color
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(Color);

    public:
        typedef uint32_t ColorVal;

        //---

        uint8_t r, g, b, a; // <- read right to left ot get the name, ABGR in this case

        //---

        INLINE Color();
        INLINE Color(const Color &other) = default;
        INLINE Color(Color&& other) = default;
        INLINE Color& operator=(const Color &other) = default;
        INLINE Color& operator=(Color&& other) = default;
        INLINE Color(uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca = 255);
        INLINE Color(ColorVal argb);

        Color(StringView txt);

        INLINE bool operator==(const Color &other) const;
        INLINE bool operator!=(const Color &other) const;

        //---

        //! Multiply color by value
        //! NOTE: we assume the color is LINEAR
        Color operator*(float scale) const;

        //! Multiply color by value
        //! NOTE: we assume the color is LINEAR
        Color &operator*=(float scale);

        //---

        // replace alpha in the color
        Color replaceAlpha(uint8_t newAlpha = 255) const;

        // scale color, leave alpha intact
        Color scaleColor(float scale) const;

        // scale color, leave alpha intact
        Color scaleAlpha(float scale) const;

        //---

        //! Get native color representation
        INLINE uint32_t toNative() const;

        //! Convert to 32-bit system color
        //! byte order in memory (LE): A, B, G, R
        INLINE uint32_t toABGR() const;

        //! Convert to 32-bit system color
        //! byte order in memory (LE): A, R, G, B
        INLINE uint32_t toARGB() const;

        //! Convert to 32-bit GPU color
        //! byte order in memory (LE): R, G, B, A
        INLINE uint32_t toRGBA() const;

        //! Convert to 32-bit GPU color
        //! byte order in memory (LE): R, G, B, A
        INLINE uint32_t toBGRA() const;

        //! Convert to 16-bit 5-6-5 color
        INLINE uint16_t toRGB565() const;

        //! Convert to 16-bit 5-6-5 color
        INLINE uint16_t toBGR565() const;

        //! Convert to 16-bit 4-4-4-4 color
        INLINE uint16_t toARGB4444() const;

        //! Convert to 16-bit 4-4-4-4 color
        INLINE uint16_t toABGR4444() const;

        //! Convert to RGBA linear vector assuming the color values are linear
        Vector4 toVectorLinear() const;

        //! Convert to RGBA linear vector assuming the color values are SRGB
        //! NOTE: alpha is not touched
        Vector4 toVectorSRGB() const;

        //! Convert to style hex represenation #FF00FF
        StringBuf toHexString(bool withAlpha = false) const;

        //---
        
        // calculate color luminance
        float luminanceSRGB() const;

        // get inverted color
        Color invertedSRGB() const;

        //---

        //! Initialize from RGBA uint32_t
        //! byte order: A, R, G, B
        INLINE static Color FromARGB(uint32_t val);

        //! Initialize from RGBA uint32_t
        //! byte order: A, B, G, R
        INLINE static Color FromABGR(uint32_t val);

        //! Initialize from RGBA uint32_t
        //! byte order: R, G, B, A
        INLINE static Color FromRGBA(uint32_t val);

        //! Initialize from RGBA uint32_t
        //! byte order: B, G, A, A
        INLINE static Color FromBGRA(uint32_t val);

        //! Initialize from RGB 565 color
        INLINE static Color FromRGB565(uint16_t val, const uint8_t alpha=255);

        //! Initialize from BGR 565 color
        INLINE static Color FromBGR565(uint16_t val, const uint8_t alpha=255);

        //! Initialize from RGB 4444 color
        INLINE static Color FromARGB4444(uint16_t val);

        //! Initialize from RGB 4444 color
        INLINE static Color FromABGR4444(uint16_t val);

        //! Initialize from linear vector
        static Color FromVectorLinear(const Vector4 &other);

        //! Initialize from linear vector to an SRGB values, fast
        static Color FromVectorSRGBFast(const Vector4 &other);

        //! Initialize from linear vector to an SRGB values, exact (uses proper lookup tables)
        static Color FromVectorSRGBExact(const Vector4 &other);

        //---

        // Parse color from HTM #FF00FF encoding, supports length 3,4,6 and 8, the "#" is dropped if present
        static bool Parse(base::StringView txt, Color& outColor);

        //---

        // typical colors, from http://www.keller.com/rgb.html
        static const ColorVal NONE;
        static const ColorVal MAROON;
        static const ColorVal DARKRED;
        static const ColorVal RED;
        static const ColorVal LIGHTPINK;
        static const ColorVal CRIMSON;
        static const ColorVal PALEVIOLETRED;
        static const ColorVal HOTPINK;
        static const ColorVal DEEPPINK;
        static const ColorVal MEDIUMVIOLETRED;
        static const ColorVal PURPLE;
        static const ColorVal DARKMAGENTA;
        static const ColorVal ORCHID;
        static const ColorVal THISTLE;
        static const ColorVal PLUM;
        static const ColorVal VIOLET;
        static const ColorVal FUCHSIA;
        static const ColorVal MAGENTA;
        static const ColorVal MEDIUMORCHID;
        static const ColorVal DARKVIOLET;
        static const ColorVal DARKORCHID;
        static const ColorVal BLUEVIOLET;
        static const ColorVal INDIGO;
        static const ColorVal MEDIUMPURPLE;
        static const ColorVal SLATEBLUE;
        static const ColorVal MEDIUMSLATEBLUE;
        static const ColorVal DARKBLUE;
        static const ColorVal MEDIUMBLUE;
        static const ColorVal BLUE;
        static const ColorVal NAVY;
        static const ColorVal MIDNIGHTBLUE;
        static const ColorVal DARKSLATEBLUE;
        static const ColorVal ROYALBLUE;
        static const ColorVal CORNFLOWERBLUE;
        static const ColorVal LIGHTSTEELBLUE;
        static const ColorVal ALICEBLUE;
        static const ColorVal GHOSTWHITE;
        static const ColorVal LAVENDER;
        static const ColorVal DODGERBLUE;
        static const ColorVal STEELBLUE;
        static const ColorVal DEEPSKYBLUE;
        static const ColorVal SLATEGRAY;
        static const ColorVal LIGHTSLATEGRAY;
        static const ColorVal LIGHTSKYBLUE;
        static const ColorVal SKYBLUE;
        static const ColorVal LIGHTBLUE;
        static const ColorVal TEAL;
        static const ColorVal DARKCYAN;
        static const ColorVal DARKTURQUOISE;
        static const ColorVal CYAN;
        static const ColorVal MEDIUMTURQUOISE;
        static const ColorVal CADETBLUE;
        static const ColorVal PALETURQUOISE;
        static const ColorVal LIGHTCYAN;
        static const ColorVal AZURE;
        static const ColorVal LIGHTSEAGREEN;
        static const ColorVal TURQUOISE;
        static const ColorVal POWDERBLUE;
        static const ColorVal DARKSLATEGRAY;
        static const ColorVal AQUAMARINE;
        static const ColorVal MEDIUMSPRINGGREEN;
        static const ColorVal MEDIUMAQUAMARINE;
        static const ColorVal SPRINGGREEN;
        static const ColorVal MEDIUMSEAGREEN;
        static const ColorVal SEAGREEN;
        static const ColorVal LIMEGREEN;
        static const ColorVal DARKGREEN;
        static const ColorVal GREEN;
        static const ColorVal LIME;
        static const ColorVal FORESTGREEN;
        static const ColorVal DARKSEAGREEN;
        static const ColorVal LIGHTGREEN;
        static const ColorVal PALEGREEN;
        static const ColorVal MINTCREAM;
        static const ColorVal HONEYDEW;
        static const ColorVal CHARTREUSE;
        static const ColorVal LAWNGREEN;
        static const ColorVal OLIVEDRAB;
        static const ColorVal DARKOLIVEGREEN;
        static const ColorVal YELLOWGREEN;
        static const ColorVal GREENYELLOW;
        static const ColorVal BEIGE;
        static const ColorVal LINEN;
        static const ColorVal LIGHTGOLDENRODYELLOW;
        static const ColorVal OLIVE;
        static const ColorVal YELLOW;
        static const ColorVal LIGHTYELLOW;
        static const ColorVal IVORY;
        static const ColorVal DARKKHAKI;
        static const ColorVal KHAKI;
        static const ColorVal PALEGOLDENROD;
        static const ColorVal WHEAT;
        static const ColorVal GOLD;
        static const ColorVal LEMONCHIFFON;
        static const ColorVal PAPAYAWHIP;
        static const ColorVal DARKGOLDENROD;
        static const ColorVal GOLDENROD;
        static const ColorVal ANTIQUEWHITE;
        static const ColorVal CORNSILK;
        static const ColorVal OLDLACE;
        static const ColorVal MOCCASIN;
        static const ColorVal NAVAJOWHITE;
        static const ColorVal ORANGE;
        static const ColorVal BISQUE;
        static const ColorVal TAN;
        static const ColorVal DARKORANGE;
        static const ColorVal BURLYWOOD;
        static const ColorVal SADDLEBROWN;
        static const ColorVal SANDYBROWN;
        static const ColorVal BLANCHEDALMOND;
        static const ColorVal LAVENDERBLUSH;
        static const ColorVal SEASHELL;
        static const ColorVal FLORALWHITE;
        static const ColorVal SNOW;
        static const ColorVal PERU;
        static const ColorVal PEACHPUFF;
        static const ColorVal CHOCOLATE;
        static const ColorVal SIENNA;
        static const ColorVal LIGHTSALMON;
        static const ColorVal CORAL;
        static const ColorVal DARKSALMON;
        static const ColorVal MISTYROSE;
        static const ColorVal ORANGERED;
        static const ColorVal SALMON;
        static const ColorVal TOMATO;
        static const ColorVal ROSYBROWN;
        static const ColorVal PINK;
        static const ColorVal INDIANRED;
        static const ColorVal LIGHTCORAL;
        static const ColorVal BROWN;
        static const ColorVal FIREBRICK;
        static const ColorVal BLACK;
        static const ColorVal DIMGRAY;
        static const ColorVal GRAY;
        static const ColorVal DARKGRAY;
        static const ColorVal SILVER;
        static const ColorVal LIGHTGREY;
        static const ColorVal GAINSBORO;
        static const ColorVal WHITESMOKE;
        static const ColorVal WHITE;
        static const ColorVal GREY;
        static const ColorVal GREY25;
        static const ColorVal GREY50;
        static const ColorVal GREY75;

        // nice pastel (crayon) color palette http://colrd.com/create/palette/?id=19308
        static const ColorVal TROVE0;
        static const ColorVal TROVE1;
        static const ColorVal TROVE2;
        static const ColorVal TROVE3;
        static const ColorVal TROVE4;
        static const ColorVal TROVE5;
        static const ColorVal TROVE6;
        static const ColorVal TROVE7;
        static const ColorVal TROVE8;
        static const ColorVal TROVE9;
        static const ColorVal TROVE10;
        static const ColorVal TROVE11;
        static const ColorVal TROVE12;
        static const ColorVal TROVE13;
        static const ColorVal TROVE14;
        static const ColorVal TROVE15;
        static const ColorVal TROVE16;
        static const ColorVal TROVE17;
        static const ColorVal TROVE18;
        static const ColorVal TROVE19;
        static const ColorVal TROVE20;

        void print(IFormatStream& f) const;
    };

} // base