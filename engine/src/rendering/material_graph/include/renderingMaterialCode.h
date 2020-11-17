/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

namespace rendering
{
    //---

    /// simplified data type for the code chunk
    enum class CodeChunkType : uint8_t
    {
        Void,
        TextureResource,
        BufferResource,
        Numerical1,
        Numerical2,
        Numerical3,
        Numerical4,
        Matrix,
    };

    //---

    enum class MaterialVertexDataType : uint8_t
    {
        ObjectID, // id of the object (uint)
        SubObjectID, // id of the sub-object (uint), usually a mesh chunk index

        VertexPosition, // unpacked vertex position in object space
        VertexNormal, // unpacked vertex normal in object space
        VertexTangent, // unpacked vertex normal in object space
        VertexBitangent, // unpacked vertex normal in object space
        VertexColor0, // per-vertex color
        VertexColor1, // per-vertex color
        VertexColor2, // per-vertex color
        VertexColor3, // per-vertex color
        VertexUV0, // texture coordinates
        VertexUV1, // texture coordinates
        VertexUV2, // texture coordinates
        VertexUV3, // texture coordinates

        WorldPosition, // absolute global world position
        WorldNormal, // global world normal
        WorldTangent, // global world tangent vector
        WorldBitangent, // global world bitangent vector

        MAX,
    };

    struct MaterialVertexDataInfo
    {
        CodeChunkType type;
        ImageFormat format;
        base::StringView shaderType;
        base::StringView name;
        bool vertexStream = false;
        bool renotmalize = false;
    };

    extern RENDERING_MATERIAL_GRAPH_API const MaterialVertexDataInfo& GetMaterialVertexDataInfo(MaterialVertexDataType type);


    //---
    
    /// a code chunk used during the material compilation
    struct RENDERING_MATERIAL_GRAPH_API CodeChunk
    {
        //--

        INLINE CodeChunk() {};
        CodeChunk(const CodeChunk& other);
        CodeChunk(CodeChunk&& other);
        CodeChunk& operator=(const CodeChunk& other);
        CodeChunk& operator=(CodeChunk&& other);

        bool operator==(const CodeChunk& other) const;
        bool operator!=(const CodeChunk& other) const;

        static const CodeChunk& EMPTY();

        INLINE base::StringView view() const { return m_text; }

        INLINE CodeChunkType type() const { return m_type; }
        INLINE const base::StringBuf& text() const { return m_text; }
        INLINE bool constant() const { return m_constant; }

        INLINE operator bool() const { return !m_text.empty(); }
        INLINE bool empty() const { return m_text.empty(); }

        // return number of components in the code chunk
        uint8_t components() const;

        //--

        CodeChunk(float val);
        CodeChunk(uint32_t val);
        CodeChunk(const base::Vector2& val, CodeChunkType type = CodeChunkType::Numerical2);
        CodeChunk(const base::Vector3& val, CodeChunkType type = CodeChunkType::Numerical3);
        CodeChunk(const base::Vector4& val, CodeChunkType type = CodeChunkType::Numerical4);
        CodeChunk(CodeChunkType type, base::StringView txt, bool isConstant = false);

        //--

        // swizzle the code chunk's data
        CodeChunk swizzle(base::StringView mask) const;

        // swizzle the code chunk's data
        CodeChunk swizzle(uint8_t inCount, uint8_t outCount, base::StringView mask) const;

        // force to given number of numerical components
        CodeChunk conform(uint32_t componentCount);

        // 1D swizzle ops
        INLINE CodeChunk x() const { return swizzle(1, 1, "x"); }
        INLINE CodeChunk y() const { return swizzle(2, 1, "y"); }
        INLINE CodeChunk z() const { return swizzle(3, 1, "z"); }
        INLINE CodeChunk w() const { return swizzle(4, 1, "w"); }

        // 2D swizzle ops
        INLINE CodeChunk xx() const { return swizzle(1, 2, "xx"); }
        INLINE CodeChunk xy() const { return swizzle(2, 2, "xy"); }
        INLINE CodeChunk xz() const { return swizzle(3, 2, "xz"); }
        INLINE CodeChunk xw() const { return swizzle(4, 2, "xw"); }
        INLINE CodeChunk yx() const { return swizzle(2, 2, "yx"); }
        INLINE CodeChunk yy() const { return swizzle(2, 2, "yy"); }
        INLINE CodeChunk yz() const { return swizzle(3, 2, "yz"); }
        INLINE CodeChunk yw() const { return swizzle(4, 2, "yw"); }
        INLINE CodeChunk zx() const { return swizzle(3, 2, "zx"); }
        INLINE CodeChunk zy() const { return swizzle(3, 2, "zy"); }
        INLINE CodeChunk zz() const { return swizzle(3, 2, "zz"); }
        INLINE CodeChunk zw() const { return swizzle(4, 2, "zw"); }
        INLINE CodeChunk wx() const { return swizzle(4, 2, "wx"); }
        INLINE CodeChunk wy() const { return swizzle(4, 2, "wy"); }
        INLINE CodeChunk wz() const { return swizzle(4, 2, "wz"); }
        INLINE CodeChunk ww() const { return swizzle(4, 2, "ww"); }

        // 3D swizzle ops
        INLINE CodeChunk xyz() const { return swizzle(3, 3, "xyz"); }
        INLINE CodeChunk xxx() const { return swizzle(1, 3, "xxx"); }
        INLINE CodeChunk yyy() const { return swizzle(2, 3, "yyy"); }
        INLINE CodeChunk zzz() const { return swizzle(3, 3, "zzz"); }
        INLINE CodeChunk www() const { return swizzle(4, 3, "www"); }

        // 4D swizzle ops (some)
        INLINE CodeChunk xyxy() const { return swizzle(2, 4, "xyxy"); }
        INLINE CodeChunk xyzw() const { return swizzle(4, 4, "xyzw"); }
        INLINE CodeChunk xxxx() const { return swizzle(1, 4, "xxxx"); }
        INLINE CodeChunk yyyy() const { return swizzle(2, 4, "yyyy"); }
        INLINE CodeChunk zzzz() const { return swizzle(3, 4, "zzzz"); }
        INLINE CodeChunk wwww() const { return swizzle(4, 4, "wwww"); }

        //--

        // conform the numerical types for binary operation (only valid for numerical types)
        // in general the number of components must match but the single float can be promoted automatically
        CodeChunkType resolveBinaryOpType(const CodeChunk& other) const;

        // create a binary operation
        CodeChunk binaryOp(const CodeChunk& other, base::StringView op) const;

        // create a binary operation with direct float value
        CodeChunk binaryOpF(base::StringView op, float value) const;

        // create a binary operation with direct float value
        CodeChunk binaryOpF(float value, base::StringView op) const;

        // simple math operators
        INLINE CodeChunk operator+(const CodeChunk& other) const { return binaryOp(other, "+"); }
        INLINE CodeChunk operator-(const CodeChunk& other) const { return binaryOp(other, "-"); }
        INLINE CodeChunk operator/(const CodeChunk& other) const { return binaryOp(other, "/"); }
        INLINE CodeChunk operator*(const CodeChunk& other) const { return binaryOp(other, "*"); }
        INLINE CodeChunk operator%(const CodeChunk& other) const { return binaryOp(other, "%"); }
        INLINE CodeChunk operator>(const CodeChunk& other) const { return binaryOp(other, ">"); }
        INLINE CodeChunk operator<(const CodeChunk& other) const { return binaryOp(other, "<"); }
        INLINE CodeChunk operator>=(const CodeChunk& other) const { return binaryOp(other, ">="); }
        INLINE CodeChunk operator<=(const CodeChunk& other) const { return binaryOp(other, "<="); }

        // direct math with floating point value
        INLINE CodeChunk operator+(float other) const { return binaryOpF("+", other); }
        INLINE CodeChunk operator-(float other) const { return binaryOpF("-", other); }
        INLINE CodeChunk operator/(float other) const { return binaryOpF("/", other); }
        INLINE CodeChunk operator*(float other) const { return binaryOpF("*", other); }
        INLINE CodeChunk operator%(float other) const { return binaryOpF("%", other); }

        //--

        // create a unary operation
        CodeChunk unaryOp(base::StringView op) const;

        // unary
        INLINE CodeChunk operator-() const { return unaryOp("-"); }

        //--

        // simple functions
        CodeChunk saturate() const;
        CodeChunk abs() const;
        CodeChunk length() const;
        CodeChunk normalize() const;
        CodeChunk sqrt() const;
        CodeChunk log() const;
        CodeChunk log2() const;
        CodeChunk exp() const;
        CodeChunk exp2() const;
        CodeChunk pow(const CodeChunk& other) const;
        CodeChunk pow(float other) const;
        CodeChunk floor() const;
        CodeChunk round() const;
        CodeChunk ceil() const;
        CodeChunk frac() const;
        CodeChunk sign() const;
        CodeChunk ddx() const;
        CodeChunk ddy() const;

        //--

        // array access operator
        CodeChunk operator[](const CodeChunk& index) const;

        //--

        // print to output text
        void print(base::IFormatStream& f) const;

    private:
        base::StringBuf m_text; // code
        CodeChunkType m_type = CodeChunkType::Void;
        bool m_constant = false;
    };

    //---

    namespace CodeChunkOp
    {
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk All(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Any(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Min(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Max(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Dot2(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Dot3(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Dot4(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Cross(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Clamp(const CodeChunk& x, const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Lerp(const CodeChunk& x, const CodeChunk& y, const CodeChunk& s);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Sign(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Step(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk SmoothStep(const CodeChunk& min, const CodeChunk& max, const CodeChunk& s);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Mad(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Sin(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Cos(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Tan(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk ArcSin(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk ArcCos(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk ArcTan(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk ArcTan2(const CodeChunk& y, const CodeChunk& x);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Reflect(const CodeChunk& v, const CodeChunk& norm);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Normalize(const CodeChunk& v);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Floor(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Ceil(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Fract(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Trunc(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Round(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float2(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float2(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float3(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float3(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float3(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float4(const CodeChunk& a);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float4(const CodeChunk& a, const CodeChunk& b);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float4(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Float4(const CodeChunk& a, const CodeChunk& b, const CodeChunk& c, const CodeChunk& d);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk DDX(const CodeChunk& uv);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk DDY(const CodeChunk& uv);

        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Tex2D(const CodeChunk& sampler, const CodeChunk& uv);
        extern RENDERING_MATERIAL_GRAPH_API CodeChunk Tex2DLod(const CodeChunk& sampler, const CodeChunk& uv, const CodeChunk& lod);

        INLINE CodeChunk operator+(float other, const CodeChunk& a) { return a.binaryOpF(other, "+"); }
        INLINE CodeChunk operator-(float other, const CodeChunk& a) { return a.binaryOpF(other, "-"); }
        INLINE CodeChunk operator/(float other, const CodeChunk& a) { return a.binaryOpF(other, "/"); }
        INLINE CodeChunk operator*(float other, const CodeChunk& a) { return a.binaryOpF(other, "*"); }

    } // CodeChunkOp

    //--

    struct MaterialOutputBlockSocketKey
    {
        const MaterialGraphBlock* block = nullptr;
        base::StringID output;

        INLINE bool operator==(const MaterialOutputBlockSocketKey& other) const
        {
            return (block == other.block) && (output == other.output);
        }

        INLINE bool operator!=(const MaterialOutputBlockSocketKey& other) const
        {
            return !operator==(other);
        }

        INLINE static uint32_t CalcHash(const MaterialOutputBlockSocketKey& key)
        {
            base::CRC32 crc;
            crc << key.block;
            crc << key.output;
            return crc;
        }
    };

    //--

    class MaterialPixelStageCompiler;
    class MaterialVertexStageCompiler;
    class MaterialDataLayout;
    struct MaterialDataLayoutEntry;    

    // compiler for single output stage - PS/VS/GS etc
    class RENDERING_MATERIAL_GRAPH_API MaterialStageCompiler : public base::StringBuilder
    {
    public:
        MaterialStageCompiler(const MaterialDataLayout* dataLayout, rendering::ShaderType stage, base::StringView materialPath, const MaterialCompilationSetup& context);

        // get compilation context
        INLINE const MaterialCompilationSetup& context() const { return m_context; }

        // name of the material being compiled (for errors)
        INLINE base::StringView path() const { return m_materialPath; }

        // get the shader stage
        INLINE ShaderType stage() const { return m_stage; }

        // data layout
        INLINE const MaterialDataLayout* dataLayout() const { return m_dataLayout; }

        // get list of additional includes
        INLINE const base::Array<base::StringBuf>& includes() const { return m_includes; }

        // emit debug code
        INLINE bool debugCode() const { return true; }

        //--

        // allocate a name
        base::StringBuf autoName();

        //--

        // allocate internal temporary variable and assign value to it
        CodeChunk var(const CodeChunk& value);

        // evaluate input to given block
        CodeChunk evalInput(const MaterialGraphBlock* sourceBlock, base::StringID socketName, const CodeChunk& defaultValue);

        //--

        // find parameter entry
        const MaterialDataLayoutEntry* findParamEntry(base::StringID entry) const;

        //--

        // request global include to be added to the file
        void includeHeader(base::StringView name);

        //--

        // request vertex shader to produce an input and to pass that input to pixel shader
        // usually it will be a vertex stream or some global stuff like "WorldPosition"
        virtual CodeChunk vertexData(MaterialVertexDataType type) = 0;

    private:
        const MaterialCompilationSetup& m_context;
        base::StringBuf m_materialPath;

        uint32_t m_autoNameCounter = 1;
        ShaderType m_stage;

        const MaterialDataLayout* m_dataLayout = nullptr;

        base::HashMap<MaterialOutputBlockSocketKey, CodeChunk> m_compiledBlockOutputs;
        base::Array<base::StringBuf> m_includes;

        bool resolveOutputBlock(const MaterialGraphBlock* sourceBlock, base::StringID socketName, const MaterialGraphBlock*& outOutputBlock, base::StringID& outOutputName, base::StringID& outOutputSwizzle) const;
    };
    
    //--
    
} // rendering
