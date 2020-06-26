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

    //-----------------------------------------------------------------------------

    INLINE Color::Color()
        : r(255), g(255), b(255), a(255)
    {}

    INLINE Color::Color(uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca/*=255*/)
        : r(cr), g(cg), b(cb), a(ca)
    {}

    INLINE Color::Color(ColorVal argb)
    {
        *this = FromARGB(argb);
    }

    INLINE bool Color::operator==(const Color &other) const
    {
        return *(const uint32_t*)this == *(const uint32_t*)&other;
    }

    INLINE bool Color::operator!=(const Color &other) const
    {
        return *(const uint32_t*)this != *(const uint32_t*)&other;
    }

    INLINE uint32_t Color::toNative() const
    {
        return *(const uint32_t *) this;
    }

    INLINE uint32_t Color::toABGR() const
    {
        uint8_t col[4] = {r, g, b, a};
        return *(uint32_t *) &col[0];
    }

    INLINE uint32_t Color::toARGB() const
    {
        uint8_t col[4] = {b, g, r, a};
        return *(uint32_t *) &col[0];
    }

    INLINE uint32_t Color::toRGBA() const
    {
        uint8_t col[4] = {a, b, g, r};
        return *(uint32_t *) &col[0];
    }

    INLINE uint32_t Color::toBGRA() const
    {
        uint8_t col[4] = {a, r, g, b};
        return *(uint32_t *) &col[0];
    }

    INLINE uint16_t Color::toRGB565() const
    {
        return (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
    }

    INLINE uint16_t Color::toBGR565() const
    {
        return (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
    }

    INLINE uint16_t Color::toABGR4444() const
    {
        return (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12);
    }

    INLINE uint16_t Color::toARGB4444() const
    {
        return (b >> 4) | ((g >> 4) << 4) | ((r >> 4) << 8) | ((a >> 4) << 12);
    }

    //-----------------------------------------------------------------------------

    INLINE Color Color::FromARGB(uint32_t val)
    {
        auto bytes  = (const uint8_t*)&val;
        return Color(bytes[2], bytes[1], bytes[0], bytes[3]);
    }

    INLINE Color Color::FromABGR(uint32_t val)
    {
        auto bytes  = (const uint8_t*)&val;
        return Color(bytes[0], bytes[1], bytes[2], bytes[3]);
    }

    INLINE Color Color::FromRGBA(uint32_t val)
    {
        auto bytes  = (const uint8_t*)&val;
        return Color(bytes[3], bytes[2], bytes[1], bytes[0]);
    }

    INLINE Color Color::FromBGRA(uint32_t val)
    {
        auto bytes  = (const uint8_t*)&val;
        return Color(bytes[1], bytes[2], bytes[3], bytes[0]);
    }

    INLINE Color Color::FromBGR565(uint16_t val, uint8_t alpha/*=255*/)
    {
        uint32_t ret = (val & 0x001F) << 3;
        ret |= (val & 0x07E0) << 5;
        ret |= (uint32_t)(val & 0xF800) << 8;
        ret |= alpha << 24;
        return Color(ret);
    }

    INLINE Color Color::FromRGB565(uint16_t val, uint8_t alpha/*=255*/)
    {
        uint32_t ret = (val & 0xF800) >> 8;
        ret |= (val & 0x7E0) << 5;
        ret |= (uint32_t)(val & 0x1F) << 19;
        ret |= alpha << 24;
        return Color(ret);
    }

    INLINE Color Color::FromARGB4444(uint16_t val)
    {
        uint32_t ret = (val & 0xF00) >> 4;
        ret |= (uint32_t)(val & 0xF0) << 8;
        ret |= (uint32_t)(val & 0xF) << 20;
        ret |= (uint32_t)(val & 0xF000) << 16;
        return Color(ret);
    }

    INLINE Color Color::FromABGR4444(uint16_t val)
    {
        uint32_t ret = (val & 0xF) << 4;
        ret |= (uint32_t)(val & 0xF0) << 8;
        ret |= (uint32_t)(val & 0xF00) << 12;
        ret |= (uint32_t)(val & 0xF000) << 16;
        return Color(ret);
    }

} // base