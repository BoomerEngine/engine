/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "replicationQuantization.h"

BEGIN_BOOMER_NAMESPACE(base::replication)

///----

// packing mode for data field
enum class PackingMode : uint8_t
{
    Default, // default packing mode
    Bit, // single bit, auto for all booleans
    Unsigned, // unsigned number limited to some bits
    Signed, // signed number limited to some bits, one extra bit is sent as sign
    RangeFloat, // range compressed floating point value
    Position, // special world position encoding (20-20-14), total: 54
    DeltaPosition, // delta position around point (11-11-11), quantized to "rangeMax", total: 33
    NormalFull, // normalized vector (16-16-1) // X Y and sign of Z, total: 33
    NormalRough, // general direction vector (4-4-1) // X Y and sign of Z, total: 9
    PitchYaw, // pitch, yaw angles pair (16-16), wrapped around to the 0-360 range, total: 32
    AllAngles, // special rotation angles encoding (12-12-8) // pitch, yaw, roll, total: 32
};

// description of replicated field
struct BASE_REPLICATION_API FieldPacking
{
    Quantization m_quantization;
    PackingMode m_mode = PackingMode::Default; // packing mode
    uint16_t m_maxLength = 0; // max length for strings
    uint16_t m_maxCount = 0; // max count for arrays
    float m_frequency = 0.0f; // replication frequency

    //--

    INLINE FieldPacking() = default;
    INLINE FieldPacking(const FieldPacking& other) = default;
    INLINE FieldPacking(FieldPacking&& other) = default;
    INLINE FieldPacking& operator=(const FieldPacking& other) = default;
    INLINE FieldPacking& operator=(FieldPacking&& other) = default;

    //--

    // parse from string directly, NOTE: asserts if invalid packing is specified
    FieldPacking(StringView txt);

    // parse from string directly, NOTE: asserts if invalid packing is specified
    static bool Parse(StringView txt, FieldPacking& outSettings);

    //--

    // print to text
    void print(IFormatStream& f) const;

    //--

    // check if we support given field as input
    bool checkTypeCompatibility(Type type, bool isPartOfArray = false) const;

    //--

    // pack data from memory into a bit stream
    void packData(const void* data, uint32_t dataSize, BitWriter& w) const;

    // unpack data from bit stream into the memory
    bool unpackData(void* data, uint32_t dataSize, BitReader& r) const;

    //--

    // calculate number of bits needed to pack the field, this is always constant for Packed fileds
    uint32_t calcBitCount() const;

public:
    static const uint32_t POSITION_X_BITS;
    static const uint32_t POSITION_Y_BITS;
    static const uint32_t POSITION_Z_BITS;
    static const float POSITION_XY_RANGE;
    static const float POSITION_Z_RANGE;

    static const Quantization POSITION_X_QUANTIZATION;
    static const Quantization POSITION_Y_QUANTIZATION;
    static const Quantization POSITION_Z_QUANTIZATION;

    static const uint32_t DELTA_POSITION_BITS;

    static const uint32_t NORMAL_X_BITS;
    static const uint32_t NORMAL_Y_BITS;

    static const Quantization NORMAL_X_QUANTIZATION;
    static const Quantization NORMAL_Y_QUANTIZATION;

    static const uint32_t FAST_NORMAL_X_BITS;
    static const uint32_t FAST_NORMAL_Y_BITS;

    static const Quantization FAST_NORMAL_X_QUANTIZATION;
    static const Quantization FAST_NORMAL_Y_QUANTIZATION;

    static const uint32_t FULL_PITCH_BITS;
    static const uint32_t FULL_YAW_BITS;

    static const Quantization FULL_PITCH_QUANTIZATION;
    static const Quantization FULL_YAW_QUANTIZATION;

    static const uint32_t SMALL_PITCH_BITS;
    static const uint32_t SMALL_YAW_BITS;
    static const uint32_t SMALL_ROLL_BITS;

    static const Quantization SMALL_PITCH_QUANTIZATION;
    static const Quantization SMALL_YAW_QUANTIZATION;
    static const Quantization SMALL_ROLL_QUANTIZATION;

    static const uint32_t SIGNED_MIN_BITS;
    static const uint32_t SIGNED_MAX_BITS;

    static const uint32_t UNSIGNED_MIN_BITS;
    static const uint32_t UNSIGNED_MAX_BITS;

    static const uint32_t FLOAT_MIN_BITS;
    static const uint32_t FLOAT_MAX_BITS;
};

///----

END_BOOMER_NAMESPACE(base::replication)
