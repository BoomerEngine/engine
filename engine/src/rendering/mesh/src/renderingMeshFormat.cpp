/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "renderingMeshFormat.h"
#include "meshopt/meshoptimizer.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ENUM(MeshVertexFormat);
        RTTI_ENUM_OPTION(PositionOnly);
        RTTI_ENUM_OPTION(Static);
        RTTI_ENUM_OPTION(StaticEx);
        RTTI_ENUM_OPTION(Skinned4);
        RTTI_ENUM_OPTION(Skinned4Ex);
    RTTI_END_TYPE();

    ///---

    struct MiniStreamInfo
    {
        const char* name = "";
        MeshStreamType stream;
        const char* unpack = "";
        ImageFormat format;
        ImageFormat readFormat;

        INLINE MiniStreamInfo(const char* name_, MeshStreamType stream_, ImageFormat type_, ImageFormat readFormat_, const char* unpack_ = "")
            : name(name_)
            , stream(stream_)
            , format(type_)
            , readFormat(readFormat_)
            , unpack(unpack_)
        {}

        INLINE MiniStreamInfo(const MiniStreamInfo& other) = default;
    };

    struct MiniFormatInfo
    {
        const char* name = "";
        std::initializer_list<MiniStreamInfo> streams;
    };

    static MiniFormatInfo GMiniFormats[(uint8_t)MeshVertexFormat::MAX] = {
        { "PositionOnly", {
            MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RGB32F, ImageFormat::RGB32F)
        }},

        { "Static", {
            //MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RGB32F, ImageFormat::RGB32F),
            //MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::R11FG11FB10F, ImageFormat::RG32_UINT, "UnpackPosition_11_11_10"),
            MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RG32_UINT, ImageFormat::RG32_UINT, "UnpackPosition_22_22_20"),
            MiniStreamInfo("VertexNormal", MeshStreamType::Normal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexTangent", MeshStreamType::Tangent_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexBitangent", MeshStreamType::Binormal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            //MiniStreamInfo("VertexUV", MeshStreamType::TexCoord0_2F, ImageFormat::RG32F, ImageFormat::RG32F),
            MiniStreamInfo("VertexUV0", MeshStreamType::TexCoord0_2F, ImageFormat::RG16F, ImageFormat::RG16F, "UnpackHalf2"),
        }},

        { "StaticEx", {
            MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RGB32F, ImageFormat::RGB32F),
            MiniStreamInfo("VertexNormal", MeshStreamType::Normal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexTangent", MeshStreamType::Tangent_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexBitangent", MeshStreamType::Binormal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexUV0", MeshStreamType::TexCoord0_2F, ImageFormat::RG32F, ImageFormat::RG32F),
            MiniStreamInfo("VertexUV1", MeshStreamType::TexCoord1_2F, ImageFormat::RG16F, ImageFormat::R32_UINT, "UnpackHalf2"),
            MiniStreamInfo("VertexColor0", MeshStreamType::Color0_4U8, ImageFormat::RGBA8_UNORM, ImageFormat::R32_UINT, "UnpackUByte4Norm"),
        }},

        { "Skinned4", {
            MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RGB32F, ImageFormat::RGB32F),
            MiniStreamInfo("VertexNormal", MeshStreamType::Normal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexTangent", MeshStreamType::Tangent_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexBitangent", MeshStreamType::Binormal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexUV0", MeshStreamType::TexCoord0_2F, ImageFormat::RG32F, ImageFormat::RG32F),
            MiniStreamInfo("SkinninIndices", MeshStreamType::SkinningIndices_4U8, ImageFormat::RGBA8_UINT, ImageFormat::R32_UINT, "UnpackSkinIndices"),
            MiniStreamInfo("SkinninWeights", MeshStreamType::SkinningWeights_4F, ImageFormat::RGBA8_UNORM, ImageFormat::R32_UINT, "UnpackSkinWeights"),
        }},

        { "Skinned4Ex", {
            MiniStreamInfo("VertexPosition", MeshStreamType::Position_3F, ImageFormat::RGB32F, ImageFormat::RGB32F),
            MiniStreamInfo("VertexNormal", MeshStreamType::Normal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexTangent", MeshStreamType::Tangent_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexBitangent", MeshStreamType::Binormal_3F, ImageFormat::R11FG11FB10F, ImageFormat::R32_UINT, "UnpackNormalVector"),
            MiniStreamInfo("VertexUV0", MeshStreamType::TexCoord0_2F, ImageFormat::RG32F, ImageFormat::RG32F),
            MiniStreamInfo("VertexUV1", MeshStreamType::TexCoord1_2F, ImageFormat::RG16F, ImageFormat::R32_UINT, "UnpackHalf2"),
            MiniStreamInfo("VertexColor0", MeshStreamType::Color0_4U8, ImageFormat::RGBA8_UNORM, ImageFormat::R32_UINT, "UnpackUByte4Norm"),
            MiniStreamInfo("SkinninIndices", MeshStreamType::SkinningIndices_4U8, ImageFormat::RGBA8_UINT, ImageFormat::R32_UINT, "UnpackSkinIndices"),
            MiniStreamInfo("SkinninWeights", MeshStreamType::SkinningWeights_4F, ImageFormat::RGBA8_UNORM, ImageFormat::R32_UINT, "UnpackSkinWeights"),
        }},
    };

    ///---

    class MeshVertexFormatRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(MeshVertexFormatRegistry);

    public:
        MeshVertexFormatRegistry()
        {
            m_streams.reserve(512);
            m_formats.reserve(64);

            buildFormatList();
        }

        INLINE const MeshVertexFormatInfo& format(MeshVertexFormat format) const
        {
            return m_formats[(uint8_t)format];
        }

        bool findFormat(base::StringView name, MeshVertexFormat& outFormat)
        {
            return false;
        }

        void addFormat(const char* name, const std::initializer_list<MiniStreamInfo>& streams)
        {
            auto& format = m_formats.emplaceBack();
            format.name = name;
            format.numStreams = streams.size();
            format.streams = m_streams.typedData() + m_streams.size();
            format.stride = 0;

            for (const auto& info : streams)
            {
                auto& newStream = m_streams.emplaceBack();
                newStream.name = info.name;
                newStream.sourceStream = info.stream;
                newStream.dataFormat = info.format;
                newStream.dataOffset = base::Align<uint32_t>(format.stride, 4);
                newStream.dataSize = GetImageFormatInfo(info.format).bitsPerPixel / 8;
                newStream.readFormat = info.readFormat;
                newStream.readFunction = info.unpack;
                format.stride = newStream.dataOffset + newStream.dataSize;
            }
        }

        void buildFormatList()
        {
            for (uint32_t i = 0; i < ARRAY_COUNT(GMiniFormats); ++i)
                addFormat(GMiniFormats[i].name, GMiniFormats[i].streams);

            TRACE_INFO("Registered {} mesh vertex formats", m_formats.size());
            for (auto& format : m_formats)
            {
                TRACE_INFO("Vertex format '{}', size {}, streams {}", format.name, format.stride, format.numStreams);

                format.quantizedPosition = false;

                for (uint32_t i = 0; i < format.numStreams; ++i)
                {
                    if (format.streams[i].sourceStream == MeshStreamType::Position_3F)
                    {
                        if (format.streams[i].dataFormat != ImageFormat::RGB32F)
                            format.quantizedPosition = true;
                    }

                   /// TRACE_INFO("Vertex format '{}', size {}, streams {}", format.name, format.stride, format.numStreams);
                }
            }

        }

    private:
        base::Array<MeshVertexStreamInfo> m_streams;
        base::Array<MeshVertexFormatInfo> m_formats;

        virtual void deinit() override
        {
            m_formats.clear();
            m_streams.clear();
        }
    };

    const MeshVertexFormatInfo& GetMeshVertexFormatInfo(MeshVertexFormat format)
    {
        return MeshVertexFormatRegistry::GetInstance().format(format);
    }

    bool GetVertexFormatByName(base::StringView name, MeshVertexFormat& outFormat)
    {
        return MeshVertexFormatRegistry::GetInstance().findFormat(name, outFormat);
    }

    INLINE static uint32_t AssembleQuantized_11_11_10(uint32_t x, uint32_t y, uint32_t z)
    {
        uint32_t ret = x;
        ret |= y << 11;
        ret |= z << 22;
        return ret;
    }

    INLINE static uint32_t AssembleQuantized_11_11_10_F(float x, float y, float z)
    {
        uint32_t qx = std::clamp<float>(std::roundf(x * 2047.0f), 0.0f, 2047.0f);
        uint32_t qy = std::clamp<float>(std::roundf(y * 2047.0f), 0.0f, 2047.0f);
        uint32_t qz = std::clamp<float>(std::roundf(z * 1023.0f), 0.0f, 1023.0f);
        return AssembleQuantized_11_11_10(qx, qy, qz);
    }

    INLINE static uint32_t AssembleQuantized_11_11_10_SF(float x, float y, float z)
    {
        uint32_t qx = std::clamp<float>(std::roundf(1023.5f + x * 1023.5f), 0.0f, 2047.0f);
        uint32_t qy = std::clamp<float>(std::roundf(1023.5f + y * 1023.5f), 0.0f, 2047.0f);
        uint32_t qz = std::clamp<float>(std::roundf(511.5f + z * 511.5f), 0.0f, 1023.0f);
        return AssembleQuantized_11_11_10(qx, qy, qz);
    }

    //--

    struct Half2
    {
        uint16_t x;
        uint16_t y;
    };

    struct Half4
    {
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint16_t w;
    };

    struct Ubyte4Unorm
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
        uint8_t w;
    };

    struct Ubyte4
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
        uint8_t w;
    };

    struct SkinningIndicesUbyte4
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
        uint8_t w;
    };

    struct SkinningWeightsUbyte4
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
        uint8_t w;
    };

    struct NormalVector
    {
        float x;
        float y;
        float z;
    };

    struct QuantizedPosition_11_11_10
    {
        uint32_t data;
    };

    struct QuantizedPosition_22_22_20
    {
        uint64_t data;
    };

    //--

    template< typename ST, typename DT>
    struct PackElement
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const ST& src, DT& dest)
        {
            ASSERT(!"Packing not supported");
        }
    };

    template<typename T>
    struct PackElement<T, T>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const T& src, T& dest)
        {
            dest = src;
        }
    };

    template<>
    struct PackElement<base::Vector2, Half2>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector2& src, Half2& dest)
        {
            dest.x = base::Float16Helper::Compress(src.x);
            dest.y = base::Float16Helper::Compress(src.y);
        }
    };
    

    template<>
    struct PackElement<base::Vector3, Ubyte4Unorm>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector3& src, Ubyte4Unorm& dest)
        {
            dest.x = base::FloatTo255(src.x);
            dest.y = base::FloatTo255(src.y);
            dest.z = base::FloatTo255(src.z);
            dest.w = 255;
        }
    };

    template<>
    struct PackElement<NormalVector, Ubyte4Unorm>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const NormalVector& src, Ubyte4Unorm& dest)
        {
            // TODO: better encoding
            dest.x = base::FloatTo255(0.5f + 0.5f * src.x);
            dest.y = base::FloatTo255(0.5f + 0.5f * src.y);
            dest.z = base::FloatTo255(0.5f + 0.5f * src.z);
            dest.w = 255;
        }
    };

    template<>
    struct PackElement<NormalVector, QuantizedPosition_11_11_10>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const NormalVector& src, QuantizedPosition_11_11_10& dest)
        {
            dest.data = AssembleQuantized_11_11_10_SF(src.x, src.y, src.z);
        }
    };

    template<>
    struct PackElement<base::Vector4, Half4>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector4& src, Half4& dest)
        {
            dest.x = base::Float16Helper::Compress(src.x);
            dest.y = base::Float16Helper::Compress(src.y);
            dest.z = base::Float16Helper::Compress(src.z);
            dest.w = base::Float16Helper::Compress(src.w);
        }
    };

    template<>
    struct PackElement<base::Vector4, Ubyte4Unorm>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector4& src, Ubyte4Unorm& dest)
        {
            dest.x = base::FloatTo255(src.x);
            dest.y = base::FloatTo255(src.y);
            dest.z = base::FloatTo255(src.z);
            dest.w = base::FloatTo255(src.w);
        }
    };

    template<>
    struct PackElement<SkinningIndicesUbyte4, Ubyte4>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const SkinningIndicesUbyte4& src, Ubyte4& dest)
        {
            dest.x = src.x;
            dest.y = src.y;
            dest.z = src.z;
            dest.w = src.w;
        }
    };

    template<>
    struct PackElement<SkinningWeightsUbyte4, Ubyte4Unorm>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const SkinningWeightsUbyte4& src, Ubyte4Unorm& dest)
        {
            dest.x = src.x;
            dest.y = src.y;
            dest.z = src.z;
            dest.w = src.w;
        }
    };

    template<>
    struct PackElement<base::Color, Ubyte4Unorm>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Color& src, Ubyte4Unorm& dest)
        {
            dest.x = src.r;
            dest.y = src.g;
            dest.z = src.b;
            dest.w = src.a;
        }
    };

    template<>
    struct PackElement<base::Vector3, QuantizedPosition_11_11_10>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector3& src, QuantizedPosition_11_11_10& dest)
        {
            dest.data = quantization.QuantizePosition_11_11_10(src);
        }
    };


    template<>
    struct PackElement<base::Vector3, QuantizedPosition_22_22_20>
    {
        INLINE static void Pack(const MeshVertexQuantizationHelper& quantization, const base::Vector3& src, QuantizedPosition_22_22_20& dest)
        {
            dest.data = quantization.QuantizePosition_22_22_20(src);
        }
    };

    //--

    template< typename ST, typename DT >
    static void PackData(const MeshVertexQuantizationHelper& quantization, const void* srcData, uint32_t srcStride, void* destData, uint32_t destStride, uint32_t count)
    {
        const uint8_t* readPtr = (const uint8_t*)srcData;
        uint8_t* writePtr = (uint8_t*)destData;

        for (uint32_t i = 0; i < count; ++i)
        {
            PackElement<ST,DT>::Pack(quantization, *(const ST*)readPtr , *(DT*)writePtr);
            readPtr += srcStride;
            writePtr += destStride;
        }
    }

    template< typename ST >
    static void PackDataT(const MeshVertexQuantizationHelper& quantization, const void* srcData, uint32_t srcStride, void* destData, uint32_t destStride, ImageFormat destFormat, uint32_t count)
    {
        switch (destFormat)
        {
            case ImageFormat::RGBA8_UNORM:
                PackData<ST, Ubyte4Unorm>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RGBA8_UINT:
                PackData<ST, Ubyte4>(quantization, srcData, srcStride, destData, destStride, count);
                break;                

            case ImageFormat::R32F:
                PackData<ST, float>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RG32F:
                PackData<ST, base::Vector2>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RGB32F:
                PackData<ST, base::Vector3>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::R11FG11FB10F:
                PackData<ST, QuantizedPosition_11_11_10>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RG32_UINT:
                PackData<ST, QuantizedPosition_22_22_20>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RGBA32F:
                PackData<ST, base::Vector4>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RG16F:
                PackData<ST, Half2>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            case ImageFormat::RGBA16F:
                PackData<ST, Half4>(quantization, srcData, srcStride, destData, destStride, count);
                break;

            default:
                ASSERT(!"Invalid packing/format combination");
        }
    }

    template< typename DT >
    void FillStreamDataT(void* destData, uint32_t destStride, const void* src, uint32_t count)
    {
        uint8_t* writePtr = (uint8_t*)destData;

        for (uint32_t i = 0; i < count; ++i)
        {
            *(DT*)writePtr = *(const DT*)src;
            writePtr += destStride;
        }
    }

    void FillStreamData(void* destData, uint32_t destStride, const void* src, ImageFormat destFormat, uint32_t count)
    {        
        switch (destFormat)
        {
        case ImageFormat::RGBA8_UINT:
        case ImageFormat::RGBA8_UNORM:
        case ImageFormat::R11FG11FB10F:
        case ImageFormat::R32F:
        case ImageFormat::RG16F:
            FillStreamDataT<uint32_t>(destData, destStride, src, count);
            break;

        case ImageFormat::RGBA16F:
        case ImageFormat::RG32F:
            FillStreamDataT<uint64_t>(destData, destStride, src, count);
            break;

        case ImageFormat::RGB32F:
            FillStreamDataT<base::Vector3>(destData, destStride, src, count);
            break;

        case ImageFormat::RGBA32F:
            FillStreamDataT<base::Vector4>(destData, destStride, src, count);
            break;

        default:
            ASSERT(!"Invalid packing/format combination");
        }
    }

    static const uint8_t FILL_ZEROS[32] = { 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
    static const uint8_t FILL_ONES[32] = { 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1 };

    void FillStreamData(void* destData, uint32_t destStride, MeshStreamType type, ImageFormat destFormat, uint32_t count)
    {
        switch (type)
        {
        case MeshStreamType::TexCoord0_2F:
        case MeshStreamType::TexCoord1_2F:
        case MeshStreamType::TexCoord2_2F:
        case MeshStreamType::TexCoord3_2F:
        case MeshStreamType::Normal_3F:
        case MeshStreamType::Tangent_3F:
        case MeshStreamType::Binormal_3F:
        case MeshStreamType::Position_3F:
            FillStreamData(destData, destStride, FILL_ZEROS, destFormat, count);
            break;

        case MeshStreamType::Color0_4U8:
        case MeshStreamType::Color1_4U8:
        case MeshStreamType::Color2_4U8:
        case MeshStreamType::Color3_4U8:
            FillStreamData(destData, destStride, FILL_ONES, destFormat, count);
            break;

        case MeshStreamType::SkinningIndices_4U8:
        case MeshStreamType::SkinningIndicesEx_4U8:
        case MeshStreamType::SkinningWeights_4F:
        case MeshStreamType::SkinningWeightsEx_4F:
            FillStreamData(destData, destStride, FILL_ZEROS, destFormat, count);
            break;

        case MeshStreamType::TreeLeafCardCorner_3F:
        case MeshStreamType::TreeLeafAnchors_3F:
        case MeshStreamType::TreeLodPosition_3F:
        case MeshStreamType::TreeLeafWindData_4F:
        case MeshStreamType::TreeFrondData_4F:
        case MeshStreamType::TreeWindBranchData_4F:
        case MeshStreamType::TreeBranchData_7F:
        case MeshStreamType::TreeLeafLod_1F:
            FillStreamData(destData, destStride, FILL_ZEROS, destFormat, count);
            break;

        case MeshStreamType::General0_F4:
        case MeshStreamType::General1_F4:
        case MeshStreamType::General2_F4:
        case MeshStreamType::General3_F4:
        case MeshStreamType::General4_F4:
        case MeshStreamType::General5_F4:
        case MeshStreamType::General6_F4:
        case MeshStreamType::General7_F4:
            FillStreamData(destData, destStride, FILL_ZEROS, destFormat, count);
            break;

        default:
            ASSERT(!"Invalid mesh stream type");
        }
    }

    void PackStreamData(const MeshVertexQuantizationHelper& quantization, const void* srcData, uint32_t srcStride, MeshStreamType type, void* destData, uint32_t destStride, ImageFormat destFormat, uint32_t count)
    {
        switch (type)
        {
        case MeshStreamType::Position_3F:
            PackDataT<base::Vector3>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::Normal_3F:
        case MeshStreamType::Tangent_3F:
        case MeshStreamType::Binormal_3F:
            PackDataT<NormalVector>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::TexCoord0_2F:
        case MeshStreamType::TexCoord1_2F:
        case MeshStreamType::TexCoord2_2F:
        case MeshStreamType::TexCoord3_2F:
            PackDataT<base::Vector2>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::Color0_4U8:
        case MeshStreamType::Color1_4U8:
        case MeshStreamType::Color2_4U8:
        case MeshStreamType::Color3_4U8:
            PackDataT<base::Color>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::SkinningIndices_4U8:
        case MeshStreamType::SkinningIndicesEx_4U8:
            PackDataT<SkinningIndicesUbyte4>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::SkinningWeights_4F:
        case MeshStreamType::SkinningWeightsEx_4F:
            PackDataT<SkinningWeightsUbyte4>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::TreeLeafCardCorner_3F:
        case MeshStreamType::TreeLeafAnchors_3F:
        case MeshStreamType::TreeLodPosition_3F:
            PackDataT<base::Vector3>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::TreeLeafWindData_4F:
        case MeshStreamType::TreeFrondData_4F:
        case MeshStreamType::TreeWindBranchData_4F:
            PackDataT<base::Vector4>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::TreeBranchData_7F:
            //PackDataT<base::Vector4>(srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::TreeLeafLod_1F:
            PackDataT<float>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        case MeshStreamType::General0_F4:
        case MeshStreamType::General1_F4:
        case MeshStreamType::General2_F4:
        case MeshStreamType::General3_F4:
        case MeshStreamType::General4_F4:
        case MeshStreamType::General5_F4:
        case MeshStreamType::General6_F4:
        case MeshStreamType::General7_F4:
            PackDataT<base::Vector4>(quantization, srcData, srcStride, destData, destStride, destFormat, count);
            break;

        default:
            ASSERT(!"Invalid mesh stream type");
        }
    }

    //--

    INLINE static uint64_t AssembleQuantized_24_24_16(uint32_t x, uint32_t y, uint32_t z)
    {
        uint64_t ret = (uint64_t)x;
        ret |= (uint64_t)y << 24;
        ret |= (uint64_t)z << 48;
        return ret;
    }

    INLINE static uint64_t AssembleQuantized_22_22_20(uint32_t x, uint32_t y, uint32_t z)
    {
        uint64_t ret = (uint64_t)x;
        ret |= (uint64_t)y << 22;
        ret |= (uint64_t)z << 44;
        return ret;
    }

    MeshVertexQuantizationHelper::MeshVertexQuantizationHelper(const base::Box& absoluteBounds)
        : m_absoluteBounds(absoluteBounds)
    {
        m_quantizationOffset = -m_absoluteBounds.min;

        auto quantizationRange = m_absoluteBounds.max - m_absoluteBounds.min;
        quantizationRange.x = std::max<float>(quantizationRange.x, 0.1f);
        quantizationRange.y = std::max<float>(quantizationRange.y, 0.1f);
        quantizationRange.z = std::max<float>(quantizationRange.z, 0.1f);

        m_quantizationScale_11_11_10.x = 2047.0f / quantizationRange.x;
        m_quantizationScale_11_11_10.y = 2047.0f / quantizationRange.y;
        m_quantizationScale_11_11_10.z = 1023.0f / quantizationRange.z;

        m_quantizationScale_22_22_20[0] = (double)((1UL << 22) - 1) / quantizationRange.x;
        m_quantizationScale_22_22_20[1] = (double)((1UL << 22) - 1) / quantizationRange.y;
        m_quantizationScale_22_22_20[2] = (double)((1UL << 20) - 1) / quantizationRange.z;
    }

    uint32_t MeshVertexQuantizationHelper::QuantizePosition_11_11_10(const base::Vector3& pos) const
    {
        auto safeX = std::clamp<float>(pos.x, m_absoluteBounds.min.x, m_absoluteBounds.max.x);
        auto safeY = std::clamp<float>(pos.y, m_absoluteBounds.min.y, m_absoluteBounds.max.y);
        auto safeZ = std::clamp<float>(pos.z, m_absoluteBounds.min.z, m_absoluteBounds.max.z);

        uint32_t quantizedX = (uint32_t)std::roundf((safeX + m_quantizationOffset.x) * m_quantizationScale_11_11_10.x);
        uint32_t quantizedY = (uint32_t)std::roundf((safeY + m_quantizationOffset.y) * m_quantizationScale_11_11_10.y);
        uint32_t quantizedZ = (uint32_t)std::roundf((safeZ + m_quantizationOffset.z) * m_quantizationScale_11_11_10.z);

        return AssembleQuantized_11_11_10(quantizedX, quantizedY, quantizedZ);
    }

    uint64_t MeshVertexQuantizationHelper::QuantizePosition_22_22_20(const base::Vector3& pos) const
    {
        auto safeX = std::clamp<float>(pos.x, m_absoluteBounds.min.x, m_absoluteBounds.max.x);
        auto safeY = std::clamp<float>(pos.y, m_absoluteBounds.min.y, m_absoluteBounds.max.y);
        auto safeZ = std::clamp<float>(pos.z, m_absoluteBounds.min.z, m_absoluteBounds.max.z);

        uint32_t quantizedX = (uint32_t)std::round((safeX + m_quantizationOffset.x) * m_quantizationScale_22_22_20[0]);
        uint32_t quantizedY = (uint32_t)std::round((safeY + m_quantizationOffset.y) * m_quantizationScale_22_22_20[1]);
        uint32_t quantizedZ = (uint32_t)std::round((safeZ + m_quantizationOffset.z) * m_quantizationScale_22_22_20[2]);

        return AssembleQuantized_22_22_20(quantizedX, quantizedY, quantizedZ);
    }

    void MeshVertexQuantizationHelper::QuantizePositions_11_11_10(void* outData, uint32_t outDataStride, const void* inData, uint32_t inputDataStride, uint32_t count) const
    {
        const auto* readPtr = (const uint8_t*)inData;
        auto* writePtr = (uint8_t*)outData;

        for (uint32_t i = 0; i < count; ++i)
        {
            *(uint32_t*)writePtr = QuantizePosition_11_11_10(*(const base::Vector3*)readPtr);
            readPtr += inputDataStride;
            writePtr += outDataStride;
        }
    }

    //--

    void PackVertexData(const MeshVertexQuantizationHelper& quantization, const SourceMeshStream* srcStreams, uint32_t srcStreamCount, void* destData, MeshVertexFormat destFormat, uint32_t count)
    {
        const auto& formatInfo = GetMeshVertexFormatInfo(destFormat);

        for (uint32_t i = 0; i < formatInfo.numStreams; ++i)
        {
            const auto& destStreamInfo = formatInfo.streams[i];
            auto* destPtr = base::OffsetPtr(destData, destStreamInfo.dataOffset);

            const void* srcPtr = nullptr;
            uint32_t srcDataStride = 0;
            for (uint32_t j = 0; j < srcStreamCount; ++j)
            {
                if (srcStreams[j].stream == destStreamInfo.sourceStream)
                {
                    srcPtr = srcStreams[j].srcData;
                    srcDataStride = srcStreams[j].srcDataStride;
                    break;
                }
            }

            if (srcPtr)
                PackStreamData(quantization, srcPtr, srcDataStride, destStreamInfo.sourceStream, destPtr, formatInfo.stride, destStreamInfo.dataFormat, count);
            else
                FillStreamData(destPtr, formatInfo.stride, destStreamInfo.sourceStream, destStreamInfo.dataFormat, count);
        }
    }

    //--

    base::Buffer UncompressVertexBuffer(const void* compressedVertexData, uint32_t compressedDataSize, MeshVertexFormat format, uint32_t count)
    {
        const auto& formatInfo = GetMeshVertexFormatInfo(format);

        base::Buffer ret;
        ret.init(POOL_TEMP, formatInfo.stride * count);

        if (0 != meshopt_decodeVertexBuffer(ret.data(), count, formatInfo.stride, (const uint8_t*)compressedVertexData, compressedDataSize))
        {
            TRACE_ERROR("Unable to decompress vertex data");
            return base::Buffer();
        }

        return ret;
    }

    base::Buffer UncompressIndexBuffer(const void* compressedIndexData, uint32_t compressedDataSize, uint32_t count)
    {
        base::Buffer ret;
        ret.init(POOL_TEMP, sizeof(uint32_t) * count);

        if (0 != meshopt_decodeIndexBuffer(ret.data(), count, sizeof(uint32_t), (const uint8_t*)compressedIndexData, compressedDataSize))
        {
            TRACE_ERROR("Unable to decompress index data");
            return base::Buffer();
        }

        return ret;
    }

    //--

} // rendering