/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationBitReader.h"
#include "replicationBitWriter.h"
#include "replicationFieldPacking.h"
#include "replicationQuantization.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/rttiArrayType.h"
#include "base/object/include/rttiProperty.h"

namespace base
{
    namespace replication
    {

        //--

        const uint32_t FieldPacking::POSITION_X_BITS = 20;
        const uint32_t FieldPacking::POSITION_Y_BITS = 20;
        const uint32_t FieldPacking::POSITION_Z_BITS = 15;
        const float FieldPacking::POSITION_XY_RANGE = 8192.0f;
        const float FieldPacking::POSITION_Z_RANGE = 512.0f;

        const Quantization FieldPacking::POSITION_X_QUANTIZATION(POSITION_X_BITS, -POSITION_XY_RANGE, POSITION_XY_RANGE);
        const Quantization FieldPacking::POSITION_Y_QUANTIZATION(POSITION_Y_BITS, -POSITION_XY_RANGE, POSITION_XY_RANGE);
        const Quantization FieldPacking::POSITION_Z_QUANTIZATION(POSITION_Z_BITS, -POSITION_Z_RANGE, POSITION_Z_RANGE);

        const uint32_t FieldPacking::DELTA_POSITION_BITS = 10;

        const uint32_t FieldPacking::NORMAL_X_BITS = 15;
        const uint32_t FieldPacking::NORMAL_Y_BITS = 15;

        const Quantization FieldPacking::NORMAL_X_QUANTIZATION(NORMAL_X_BITS, -1.0f, 1.0f);
        const Quantization FieldPacking::NORMAL_Y_QUANTIZATION(NORMAL_Y_BITS, -1.0f, 1.0f);

        const uint32_t FieldPacking::FAST_NORMAL_X_BITS = 4;
        const uint32_t FieldPacking::FAST_NORMAL_Y_BITS = 4;

        const Quantization FieldPacking::FAST_NORMAL_X_QUANTIZATION(FAST_NORMAL_X_BITS, -1.0f, 1.0f);
        const Quantization FieldPacking::FAST_NORMAL_Y_QUANTIZATION(FAST_NORMAL_Y_BITS, -1.0f, 1.0f);

        const uint32_t FieldPacking::FULL_PITCH_BITS = 16;
        const uint32_t FieldPacking::FULL_YAW_BITS = 16;

        const Quantization FieldPacking::FULL_PITCH_QUANTIZATION(FULL_PITCH_BITS, 0.0f, 360.0f);
        const Quantization FieldPacking::FULL_YAW_QUANTIZATION(FULL_YAW_BITS, 0.0f, 360.0f);

        const uint32_t FieldPacking::SMALL_PITCH_BITS = 10;
        const uint32_t FieldPacking::SMALL_YAW_BITS = 10;
        const uint32_t FieldPacking::SMALL_ROLL_BITS = 8;

        const Quantization FieldPacking::SMALL_PITCH_QUANTIZATION(SMALL_PITCH_BITS, 0.0f, 360.0f);
        const Quantization FieldPacking::SMALL_YAW_QUANTIZATION(SMALL_YAW_BITS, 0.0f, 360.0f);
        const Quantization FieldPacking::SMALL_ROLL_QUANTIZATION(SMALL_ROLL_BITS, 0.0f, 360.0f);

        const uint32_t FieldPacking::SIGNED_MIN_BITS = 2;
        const uint32_t FieldPacking::SIGNED_MAX_BITS = 24;

        const uint32_t FieldPacking::UNSIGNED_MIN_BITS = 1;
        const uint32_t FieldPacking::UNSIGNED_MAX_BITS = 24;

        const uint32_t FieldPacking::FLOAT_MIN_BITS = 4;
        const uint32_t FieldPacking::FLOAT_MAX_BITS = 16;

        //--

        FieldPacking::FieldPacking(StringView<char> txt)
        {
            if (!Parse(txt, *this))
            {
                FATAL_ERROR("Unable to parser packing settings");
            }
        }

        bool FieldPacking::Parse(StringView<char> txt, FieldPacking& outFieldPacking)
        {
            StringParser p(txt);

            if (txt.empty())
            {
                outFieldPacking = FieldPacking();
                return true;
            }

            FieldPacking ret;
            while (p.parseWhitespaces())
            {
                p.parseKeyword(",");

                StringView<char> key;
                if (!p.parseString(key, ":,"))
                {
                    TRACE_ERROR("Expected identifier");
                    return false;
                }

                // parse values
                if (key == "b")
                {
                    ret.m_mode = PackingMode::Bit;
                    ret.m_quantization = Quantization(1);
                }
                else if (key == "u")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    uint8_t bitCount = 0;
                    if (!p.parseUint8(bitCount))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (bitCount < UNSIGNED_MIN_BITS)
                    {
                        TRACE_ERROR("Unsigned data requires at least {} bits", UNSIGNED_MIN_BITS);
                        return false;
                    }

                    if (bitCount > UNSIGNED_MAX_BITS)
                    {
                        TRACE_ERROR("Unsigned data can be at most {} bits", UNSIGNED_MAX_BITS);
                        return false;
                    }

                    ret.m_mode = PackingMode::Unsigned;
                    ret.m_quantization = Quantization(bitCount);
                }
                else if (key == "s")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    uint8_t bitCount = 0;
                    if (!p.parseUint8(bitCount))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (bitCount < SIGNED_MIN_BITS)
                    {
                        TRACE_ERROR("Signed data requires at least {} bits", SIGNED_MIN_BITS);
                        return false;
                    }

                    if (bitCount > SIGNED_MAX_BITS)
                    {
                        TRACE_ERROR("Signed data can be at most {} bits", SIGNED_MAX_BITS);
                        return false;
                    }

                    ret.m_mode = PackingMode::Signed;
                    ret.m_quantization = Quantization(bitCount);
                }
                else if (key == "f")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    uint8_t bitCount = 0;
                    if (!p.parseUint8(bitCount))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (!p.parseKeyword(","))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    float minValue = 0.0f;
                    if (!p.parseFloat(minValue))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (!p.parseKeyword(","))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    float maxValue = 0.0f;
                    if (!p.parseFloat(maxValue))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (bitCount < FLOAT_MIN_BITS)
                    {
                        TRACE_ERROR("float data requires at least {} bits", FLOAT_MIN_BITS);
                        return false;
                    }

                    if (bitCount > FLOAT_MAX_BITS)
                    {
                        TRACE_ERROR("float data can be at most {} bits", FLOAT_MAX_BITS);
                        return false;
                    }

                    if (minValue >= maxValue)
                    {
                        TRACE_ERROR("Invalid float range specified");
                        return false;
                    }

                    ret.m_mode = PackingMode::RangeFloat;
                    ret.m_quantization = Quantization(bitCount, minValue, maxValue);
                }
                else if (key == "pos")
                {
                    ret.m_mode = PackingMode::Position;
                }
                else if (key == "delta")
                {
                    ret.m_mode = PackingMode::DeltaPosition;

                    if (!p.parseKeyword(","))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    float max = 0.0f;
                    if (!p.parseFloat(max))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }

                    if (max <= 0.0f)
                    {
                        TRACE_ERROR("Invalid float range specified");
                        return false;
                    }

                    ret.m_quantization = Quantization(DELTA_POSITION_BITS, -max, max);
                }
                else if (key == "normal")
                {
                    ret.m_mode = PackingMode::NormalFull;
                }
                else if (key == "dir")
                {
                    ret.m_mode = PackingMode::NormalRough;
                }
                else if (key == "pitchYaw")
                {
                    ret.m_mode = PackingMode::PitchYaw;
                }
                else if (key == "angles")
                {
                    ret.m_mode = PackingMode::AllAngles;
                }
                else if (key == "freq")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    if (!p.parseFloat(ret.m_frequency))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }
                }
                else if (key == "maxCount")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    if (!p.parseUint16(ret.m_maxCount))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }
                }
                else if (key == "maxLength")
                {
                    if (!p.parseKeyword(":"))
                    {
                        TRACE_ERROR("Expected : after {}", key);
                        return false;
                    }

                    if (!p.parseUint16(ret.m_maxLength))
                    {
                        TRACE_ERROR("Unable to parser value for {}", key);
                        return false;
                    }
                }
                else
                {
                    TRACE_ERROR("Unrecognized field option: '{}'", key);
                    return false;
                }
            }

            outFieldPacking = ret;
            return true;
        }

        void FieldPacking::print(IFormatStream& f) const
        {
            switch (m_mode)
            {
                case PackingMode::Bit: f << "b"; break;
                case PackingMode::Unsigned: f << "u"; break;
                case PackingMode::Signed: f << "s"; break;
                case PackingMode::RangeFloat: f << "f"; break;
                case PackingMode::Position: f << "pow"; break;
                case PackingMode::DeltaPosition: f << "delta"; break;
                case PackingMode::NormalFull: f << "normal"; break;
                case PackingMode::NormalRough: f << "dir"; break;
                case PackingMode::PitchYaw: f << "pitchYaw"; break;
                case PackingMode::AllAngles: f << "angles"; break;
            }

            if (m_mode == PackingMode::Default && m_maxLength > 0)
                f.appendf("maxLength:{}", m_maxLength);

            if (m_mode == PackingMode::Signed || m_mode == PackingMode::Unsigned || m_mode == PackingMode::RangeFloat)
                f.appendf(",{}", m_quantization.m_bitCount.m_bitCount);

            if (m_mode == PackingMode::RangeFloat)
                f.appendf(",{},{}", m_quantization.m_quantizationMin, m_quantization.m_quantizationMax);
            else if (m_mode == PackingMode::Default && m_quantization.m_quantizationMax > 0.0f)
                f.appendf(",{}", m_quantization.m_quantizationMax);

            if (m_maxCount > 0)
                f.appendf(",maxCount:{}", m_maxCount);

            if (m_frequency > 0.0f)
                f.appendf(",freq:{}", m_frequency);
        }

        //--

        uint32_t CountFloatsInStruct(ClassType type)
        {
            uint32_t numFloats = 0;
            uint32_t expectedOffset = 0;
            for (auto prop  : type->allProperties())
            {
                if (prop->offset() != expectedOffset)
                    return 0;

                if (prop->type() != reflection::GetTypeObject<float>())
                    return 0;

                numFloats += 1;
                expectedOffset += 4;
            }

            return numFloats;
        }

        uint32_t CountFloatsInType(Type type)
        {
            if (type == reflection::GetTypeObject<float>())
                return 1;

            if (type->metaType() == rtti::MetaType::Class)
                return CountFloatsInStruct(type.toClass());

            return 0;
        }

        bool FieldPacking::checkTypeCompatibility(Type type, bool isPartOfArray /*= false*/) const
        {
            // array can be saved if we can deal with the members
            if (type.isArray())
            {
                // we support only single-level arrays
                if (isPartOfArray)
                    return false;

                return checkTypeCompatibility(type.innerType(), true);
            }

            switch (m_mode)
            {
                case PackingMode::Default:
                    if (type->size() > 8) return false;
                    if (type == reflection::GetTypeObject<StringBuf>()) return false;
                    if (type == reflection::GetTypeObject<StringID>()) return false;
                    if (type->metaType() == rtti::MetaType::Simple) return true;
                    if (type->metaType() == rtti::MetaType::Enum) return true;
                    return false;

                case PackingMode::Bit:
                    return type == reflection::GetTypeObject<bool>();

                case PackingMode::Unsigned:
                    if (type == reflection::GetTypeObject<uint8_t>() && m_quantization.m_bitCount.m_bitCount <= 8) return true;
                    if (type == reflection::GetTypeObject<uint16_t>() && m_quantization.m_bitCount.m_bitCount <= 16) return true;
                    if (type == reflection::GetTypeObject<uint32_t>() && m_quantization.m_bitCount.m_bitCount <= 32) return true;
                    if (type == reflection::GetTypeObject<uint64_t>() && m_quantization.m_bitCount.m_bitCount <= 64) return true;
                    return false;

                case PackingMode::Signed:
                    if (type == reflection::GetTypeObject<char>() && m_quantization.m_bitCount.m_bitCount <= 8) return true;
                    if (type == reflection::GetTypeObject<short>() && m_quantization.m_bitCount.m_bitCount <= 16) return true;
                    if (type == reflection::GetTypeObject<int>() && m_quantization.m_bitCount.m_bitCount <= 32) return true;
                    if (type == reflection::GetTypeObject<int64_t>() && m_quantization.m_bitCount.m_bitCount <= 64) return true;
                    return false;

                case PackingMode::RangeFloat:
                    return CountFloatsInType(type) >= 1; // we can quantize all floats in structure with the same settings

                case PackingMode::NormalFull:
                case PackingMode::NormalRough:
                case PackingMode::DeltaPosition:
                case PackingMode::Position:
                case PackingMode::AllAngles:
                    return CountFloatsInType(type) == 3;

                case PackingMode::PitchYaw:
                    return CountFloatsInType(type) == 2 || CountFloatsInType(type) == 3;
            }

            return false;
        }

        //--

        uint32_t FieldPacking::calcBitCount() const
        {
            switch (m_mode)
            {
                case PackingMode::Bit:
                    return 1;

                case PackingMode::RangeFloat:
                case PackingMode::Signed:
                case PackingMode::Unsigned:
                    return m_quantization.m_bitCount.m_bitCount;

                case PackingMode::Position:
                    return POSITION_X_BITS + POSITION_Y_BITS + POSITION_Z_BITS;

                case PackingMode::DeltaPosition:
                    return 3*DELTA_POSITION_BITS;

                case PackingMode::NormalFull:
                    return 1 + NORMAL_Y_BITS + NORMAL_X_BITS;

                case PackingMode::NormalRough:
                    return 1 + FAST_NORMAL_Y_BITS + FAST_NORMAL_X_BITS;

                case PackingMode::PitchYaw:
                    return FULL_PITCH_BITS + FULL_YAW_BITS;

                case PackingMode::AllAngles:
                    return SMALL_YAW_BITS + SMALL_PITCH_BITS + SMALL_ROLL_BITS;
            }

            ASSERT(!"Invalid packing mode");
            return 0;
        }

        //--

        template< typename T >
        const T& GetFieldValue(const void* fieldData, uint32_t offset = 0)
        {
            return *(const T*)((const uint8_t*)fieldData + offset);
        }

        INLINE static void WriteQuantizedUnsigned(const Quantization& q, uint64_t v, BitWriter& w)
        {
            auto qv = q.quantizeUnsigned((uint32_t)v);
            w.writeBits(qv, q.m_bitCount.m_bitCount);
        }

        INLINE static void WriteQuantizedSigned(const Quantization& q, int64_t v, BitWriter& w)
        {
            auto qv = q.quantizeSigned((int)v);
            w.writeBits(qv, q.m_bitCount.m_bitCount);
        }

        INLINE static void WriteQuantizedFloat(const Quantization& q, float v, BitWriter& w)
        {
            auto qv = q.quantizeFloat(v);
            w.writeBits(qv, q.m_bitCount.m_bitCount);
        }

        void FieldPacking::packData(const void* data, uint32_t dataSize, BitWriter& w) const
        {
            switch (m_mode)
            {
                case PackingMode::Default:
                {
                    if (dataSize <= 4)
                    {
                        uint32_t value = 0;
                        switch (dataSize)
                        {
                            case 1: value = GetFieldValue<uint8_t>(data); break;
                            case 2: value = GetFieldValue<uint16_t>(data); break;
                            case 4: value = GetFieldValue<uint32_t>(data); break;
                        }

                        w.writeBits(value, dataSize * 8);
                    }
                    else
                    {
                        w.align();
                        w.writeBlock(data, dataSize);
                    }
                    break;
                }

               case PackingMode::Bit:
               {
                   auto& value = GetFieldValue<bool>(data);
				   w.writeBit(value);
                   break;
               }

               case PackingMode::Unsigned:
               {
                   uint64_t value = 0;
                   switch (dataSize)
                   {
                       case 1: value = GetFieldValue<uint8_t>(data); break;
                       case 2: value = GetFieldValue<uint16_t>(data); break;
                       case 4: value = GetFieldValue<uint32_t>(data); break;
                       case 8: value = GetFieldValue<uint64_t>(data); break;
                   }

                   WriteQuantizedUnsigned(m_quantization, value, w);
                   break;
               }

               case PackingMode::Signed:
               {
                   int64_t value = 0;
                   switch (dataSize)
                   {
                       case 1: value = GetFieldValue<char>(data); break;
                       case 2: value = GetFieldValue<short>(data); break;
                       case 4: value = GetFieldValue<int>(data); break;
                       case 8: value = GetFieldValue<int64_t>(data); break;
                   }

                   WriteQuantizedSigned(m_quantization, value, w);
                   break;
               }

               case PackingMode::RangeFloat:
               {
                   auto value = GetFieldValue<float>(data);
                   WriteQuantizedFloat(m_quantization, value, w);
                   break;
               }

               case PackingMode::Position:
               {
                   auto rawX = GetFieldValue<float>(data, 0);
                   auto rawY = GetFieldValue<float>(data, 4);
                   auto rawZ = GetFieldValue<float>(data, 8);

                   WriteQuantizedFloat(POSITION_X_QUANTIZATION, rawX, w);
                   WriteQuantizedFloat(POSITION_Y_QUANTIZATION, rawY, w);
                   WriteQuantizedFloat(POSITION_Z_QUANTIZATION, rawZ, w);
                   break;
               }

               case PackingMode::DeltaPosition:
               {
                   auto rawX = GetFieldValue<float>(data, 0);
                   auto rawY = GetFieldValue<float>(data, 4);
                   auto rawZ = GetFieldValue<float>(data, 8);

                   WriteQuantizedFloat(m_quantization, rawX, w);
                   WriteQuantizedFloat(m_quantization, rawY, w);
                   WriteQuantizedFloat(m_quantization, rawZ, w);
                   break;
               }

               case PackingMode::NormalFull:
               {
                   auto rawX = GetFieldValue<float>(data, 0);
                   auto rawY = GetFieldValue<float>(data, 4);
                   auto rawZ = GetFieldValue<float>(data, 8);

                   float len = rawX*rawX + rawY*rawY + rawZ*rawZ;
                   if (len > 0.00001f)
                   {
                       float inv = 1.0f / std::sqrt(len);
                       rawX *= inv;
                       rawY *= inv;
                       rawZ *= inv;
                   }

                   WriteQuantizedFloat(NORMAL_X_QUANTIZATION, rawX, w);
                   WriteQuantizedFloat(NORMAL_Y_QUANTIZATION, rawY, w);
                   w.writeBit(rawZ < 0.0f);
                   break;
               }

               case PackingMode::NormalRough:
               {
                   auto rawX = GetFieldValue<float>(data, 0);
                   auto rawY = GetFieldValue<float>(data, 4);
                   auto rawZ = GetFieldValue<float>(data, 8);

                   float len = rawX*rawX + rawY*rawY + rawZ*rawZ;
                   if (len > 0.00001f)
                   {
                       float inv = 1.0f / std::sqrt(len);
                       rawX *= inv;
                       rawY *= inv;
                       rawZ *= inv;
                   }

                   WriteQuantizedFloat(FAST_NORMAL_X_QUANTIZATION, rawX, w);
                   WriteQuantizedFloat(FAST_NORMAL_Y_QUANTIZATION, rawY, w);
                   w.writeBit(rawZ < 0.0f);
                   break;
               }

               case PackingMode::PitchYaw:
               {
                   auto rawPitch = std::fmod(GetFieldValue<float>(data, 0), 360.0f);
                   auto rawYaw = std::fmod(GetFieldValue<float>(data, 4), 360.0f);

                   if (rawPitch < 0.0f)
                       rawPitch += 360.0f;

                   if (rawYaw < 0.0f)
                        rawYaw += 360.0f;

                   WriteQuantizedFloat(FULL_PITCH_QUANTIZATION, rawPitch, w);
                   WriteQuantizedFloat(FULL_YAW_QUANTIZATION, rawYaw, w);
                   break;
               }

               case PackingMode::AllAngles:
               {
                   auto rawPitch = std::fmod(GetFieldValue<float>(data, 0), 360.0f);
                   auto rawYaw = std::fmod(GetFieldValue<float>(data, 4), 360.0f);
                   auto rawRoll = std::fmod(GetFieldValue<float>(data, 8), 360.0f);

                   if (rawPitch < 0.0f)
                       rawPitch += 360.0f;

                   if (rawYaw < 0.0f)
                       rawYaw += 360.0f;

                   if (rawRoll < 0.0f)
                       rawRoll += 360.0f;

                   WriteQuantizedFloat(SMALL_PITCH_QUANTIZATION, rawPitch, w);
                   WriteQuantizedFloat(SMALL_YAW_QUANTIZATION, rawYaw, w);
                   WriteQuantizedFloat(SMALL_ROLL_QUANTIZATION, rawRoll, w);
                   break;
                }

                default:
                {
                    ASSERT(!"Invalid packing mode");
                }
            }
        }

        //--

        template< typename T >
        T& GetFieldValue(void* fieldData, uint32_t offset = 0)
        {
            return *(T*)((uint8_t*)fieldData + offset);
        }

        INLINE static bool ReadQuantizedUnsigned(const Quantization& q, BitReader& r, uint32_t& outValue)
        {
            BitReader::WORD word = 0;
            if (!r.readBits(q.m_bitCount.m_bitCount, word))
                return false;

            outValue = q.unquantizeUnsigned(word);
            return true;
        }

        INLINE static bool ReadQuantizedSigned(const Quantization& q, BitReader& r, int& outValue)
        {
            BitReader::WORD word = 0;
            if (!r.readBits(q.m_bitCount.m_bitCount, word))
                return false;

            outValue = q.unquantizeSigned(word);
            return true;
        }

        INLINE static bool ReadQuantizedFloat(const Quantization& q, BitReader& r, float& outValue)
        {
            BitReader::WORD word = 0;
            if (!r.readBits(q.m_bitCount.m_bitCount, word))
                return false;

            outValue = q.unquantizeFloat(word);
            return true;
        }

        bool FieldPacking::unpackData(void* data, uint32_t dataSize, BitReader& r) const
        {
            switch (m_mode)
            {
                case PackingMode::Default:
                {
                    if (dataSize <= 4)
                    {
                        BitReader::WORD value = 0;
                        if (!r.readBits(dataSize * 8, value))
                            return false;

                        switch (dataSize)
                        {
                            case 1: GetFieldValue<uint8_t>(data) = (uint8_t)value; break;
                            case 2: GetFieldValue<uint16_t>(data) = (uint16_t)value; break;
                            case 4: GetFieldValue<uint32_t>(data) = (uint32_t)value; break;
                        }
                    }
                    else
                    {
                        r.align();
                        if (!r.readBlock(data, dataSize))
                            return false;
                    }
                    break;
                }

                case PackingMode::Bit:
                {
                    bool bit = false;
					auto offset = r.bitPos();
                    if (!r.readBit(bit))
                        return false;

                    GetFieldValue<bool>(data) = bit;
                    break;
                }

                case PackingMode::Unsigned:
                {
                    uint32_t value = 0;
                    if (!ReadQuantizedUnsigned(m_quantization, r, value))
                        return false;

                    switch (dataSize)
                    {
                        case 1: GetFieldValue<uint8_t>(data) = (uint8_t)value; break;
                        case 2: GetFieldValue<uint16_t>(data) = (uint16_t)value; break;
                        case 4: GetFieldValue<uint32_t>(data) = (uint32_t)value; break;
                        case 8: GetFieldValue<uint64_t>(data) = (uint64_t)value; break;
                    }
                    break;
                }

                case PackingMode::Signed:
                {
                    int value = 0;
                    if (!ReadQuantizedSigned(m_quantization, r, value))
                        return false;

                    switch (dataSize)
                    {
                        case 1: GetFieldValue<char>(data) = (char)value; break;
                        case 2: GetFieldValue<short>(data) = (short)value; break;
                        case 4: GetFieldValue<int>(data) = (int)value; break;
                        case 8: GetFieldValue<int64_t>(data) = (int64_t)value; break;
                    }
                    break;
                }

                case PackingMode::RangeFloat:
                {
                    if (!ReadQuantizedFloat(m_quantization, r, GetFieldValue<float>(data)))
                        return false;
                    break;
                }

                case PackingMode::Position:
                {
                    if (!ReadQuantizedFloat(POSITION_X_QUANTIZATION, r, GetFieldValue<float>(data, 0)))
                        return false;
                    if (!ReadQuantizedFloat(POSITION_Y_QUANTIZATION, r, GetFieldValue<float>(data, 4)))
                        return false;
                    if (!ReadQuantizedFloat(POSITION_Z_QUANTIZATION, r, GetFieldValue<float>(data, 8)))
                        return false;
                    break;
                }

                case PackingMode::DeltaPosition:
                {
                    if (!ReadQuantizedFloat(m_quantization, r, GetFieldValue<float>(data, 0)))
                        return false;
                    if (!ReadQuantizedFloat(m_quantization, r, GetFieldValue<float>(data, 4)))
                        return false;
                    if (!ReadQuantizedFloat(m_quantization, r, GetFieldValue<float>(data, 8)))
                        return false;
                    break;
                }

                case PackingMode::NormalFull:
                {
                    float nx=0.0f, ny = 0.0f;
                    if (!ReadQuantizedFloat(NORMAL_X_QUANTIZATION, r, nx))
                        return false;
                    if (!ReadQuantizedFloat(NORMAL_Y_QUANTIZATION, r, ny))
                        return false;

                    bool sz = false;
                    if (!r.readBit(sz))
                        return false;

                    auto nz = std::sqrt(1.0f - (nx*nx + ny*ny));
                    if (sz)
                        nz = -nz;

                    GetFieldValue<float>(data, 0) = nx;
                    GetFieldValue<float>(data, 4) = ny;
                    GetFieldValue<float>(data, 8) = nz;
                    break;
                }

                case PackingMode::NormalRough:
                {
                    float nx=0.0f, ny = 0.0f;
                    if (!ReadQuantizedFloat(FAST_NORMAL_X_QUANTIZATION, r, nx))
                        return false;
                    if (!ReadQuantizedFloat(FAST_NORMAL_Y_QUANTIZATION, r, ny))
                        return false;

                    bool sz = false;
                    if (!r.readBit(sz))
                        return false;

                    auto nz = std::sqrt(1.0f - (nx*nx + ny*ny));
                    if (sz)
                        nz = -nz;

                    GetFieldValue<float>(data, 0) = nx;
                    GetFieldValue<float>(data, 4) = ny;
                    GetFieldValue<float>(data, 8) = nz;
                    break;
                }

                case PackingMode::PitchYaw:
                {
                    if (!ReadQuantizedFloat(FULL_PITCH_QUANTIZATION, r, GetFieldValue<float>(data, 0)))
                        return false;
                    if (!ReadQuantizedFloat(FULL_YAW_QUANTIZATION, r, GetFieldValue<float>(data, 4)))
                        return false;
                    break;
                }

                case PackingMode::AllAngles:
                {
                    if (!ReadQuantizedFloat(SMALL_PITCH_QUANTIZATION, r, GetFieldValue<float>(data, 0)))
                         return false;
                    if (!ReadQuantizedFloat(SMALL_YAW_QUANTIZATION, r, GetFieldValue<float>(data, 4)))
                        return false;
                    if (!ReadQuantizedFloat(SMALL_ROLL_QUANTIZATION, r, GetFieldValue<float>(data, 8)))
                        return false;
                    break;
                }

                default:
                {
                    return false; // invalid packing mode
                }
            }

            return true;
        }

    } // replication
} // base