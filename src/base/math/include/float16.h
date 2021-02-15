/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\float16 #]
***/

#pragma once

namespace base
{

    //--

    /// Specialized class to convert between 32bit float and 16bit float
    class BASE_MATH_API Float16Helper
    {
        union Bits
        {
            float f;
            int si;
            uint32_t ui;
        };

        static int const shift = 13;
        static int const shiftSign = 16;

        static int const infN = 0x7F800000; // flt32 infinity
        static int const maxN = 0x477FE000; // max flt16 normal as a flt32
        static int const minN = 0x38800000; // min flt16 normal as a flt32
        static int const signN = 0x80000000; // flt32 sign bit

        static int const infC = infN >> shift;
        static int const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
        static int const maxC = maxN >> shift;
        static int const minC = minN >> shift;
        static int const signC = signN >> shiftSign; // flt16 sign bit

        static int const mulN = 0x52000000; // (1 << 23) / minN
        static int const mulC = 0x33800000; // minN / (1 << (23 - shift))

        static int const subC = 0x003FF; // max flt32 subnormal down shifted
        static int const norC = 0x00400; // min flt32 normal down shifted

        static int const maxD = infC - maxC - 1;
        static int const minD = minC - subC - 1;

    public:
        //! Convert from 32-bit float to 16-bit float ( saved in uint16_t )
        static uint16_t Compress(float value);

        //! Convert from 16-bit float to 32-bit float
        static float Decompress(uint16_t value);
    };

    // Float16 class wrapper
    class BASE_MATH_API Float16
    {
    public:
        INLINE Float16()
            : m_value(0)
        {}

        INLINE Float16(float val)
        {
            m_value = Float16Helper::Compress(val);
        }

        INLINE Float16(const Float16& other) = default;
        INLINE Float16(Float16&& other) = default;
        INLINE Float16& operator=(const Float16& other) = default;
        INLINE Float16& operator=(Float16&& other) = default;

        INLINE float value() const
        {
            return Float16Helper::Decompress(m_value);
        }

        INLINE bool operator==(Float16 other) const
        {
            return m_value == other.m_value;
        }

        INLINE bool operator!=(Float16 other) const
        {
            return m_value != other.m_value;
        }

        INLINE bool operator<(Float16 other) const
        {
            return m_value < other.m_value;
        }

        INLINE bool operator<=(Float16 other) const
        {
            return m_value <= other.m_value;
        }

        INLINE bool operator>(Float16 other) const
        {
            return m_value > other.m_value;
        }

        INLINE bool operator>=(Float16 other) const
        {
            return m_value >=other.m_value;
        }

        INLINE Float16 operator-() const
        {
            Float16 ret(*this);
            ret.m_value ^= 0x8000;
            return ret;
        }

    private:
        uint16_t m_value;
    };

    //---

} // base
