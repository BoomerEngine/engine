/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\constants #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_ConstColor : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstColor, MaterialGraphBlock);

    public:
        MaterialGraphBlock_ConstColor()
            : m_color(255,255,255,255)
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
            builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault().color(COLOR_SOCKET_RED));
            builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault().color(COLOR_SOCKET_GREEN));
            builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault().color(COLOR_SOCKET_BLUE));
            builder.socket("A"_id, MaterialOutputSocket().swizzle("w"_id).color(COLOR_SOCKET_ALPHA));
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            if (m_linearColor)
                return CodeChunk(m_color.toVectorLinear());
            else
                return CodeChunk(m_color.toVectorSRGB());
        }

    private:
        base::Color m_color;
        bool m_linearColor = false;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstColor);
        RTTI_METADATA(base::graph::BlockInfoMetadata).name("Constant color").group("Constants").title("Color");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialConst");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
        RTTI_PROPERTY(m_color).editable("Constant color");
        RTTI_PROPERTY(m_linearColor).editable("Color is in linear space, no sRGB conversion is required");
    RTTI_END_TYPE();

    ///---

} // rendering
