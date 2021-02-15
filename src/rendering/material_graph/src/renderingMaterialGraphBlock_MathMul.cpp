/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_MathMul : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMul, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
            builder.socket("A"_id, MaterialInputSocket().hideCaption());
            builder.socket("B"_id, MaterialInputSocket().hideCaption());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk a = compiler.evalInput(this, "A"_id, m_defaultA);
            CodeChunk b = compiler.evalInput(this, "B"_id, m_defaultB);

            const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
            a = a.conform(maxComponents);
            b = b.conform(maxComponents);

            return a * b;
        }

        float m_defaultA = 1.0f;
        float m_defaultB = 1.0f;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMul);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("a * b").group("Math").name("Multiply");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
        RTTI_PROPERTY(m_defaultA).editable();
        RTTI_PROPERTY(m_defaultB).editable();
    RTTI_END_TYPE();

    ///---

} // rendering
