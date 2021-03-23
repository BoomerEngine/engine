/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "structurePanel.h"
#include "structureNodes.h"
#include "previewPanel.h"

#include "engine/mesh/include/mesh.h"
#include "engine/material/include/material.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/texture/include/texture.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(IMeshStructureNode);
RTTI_END_TYPE();

IMeshStructureNode::IMeshStructureNode(Mesh* mesh, StringView text)
    : m_mesh(AddRef(mesh))
{
    createChild<ui::TextLabel>(text);
}

IMeshStructureNode::~IMeshStructureNode()
{}

void IMeshStructureNode::handleItemExpand()
{
}

void IMeshStructureNode::handleItemCollapse()
{
    removeAllChildren();
}

void IMeshStructureNode::printDetails(IFormatStream& f) const
{
    f << "No additional details";
}

void IMeshStructureNode::setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const
{

}

void IMeshStructureNode::render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const
{

}

//--

class MeshStructore_MaterialShader : public IMeshStructureNode
{
public:
    MeshStructore_MaterialShader(Mesh* mesh, MaterialTemplateRef temp)
        : IMeshStructureNode(mesh, TempString("[img:shader] Template: {}", temp.resource()->loadPath().view().fileStem()))
        , m_template(temp)
    {}

    virtual void printDetails(IFormatStream& f) const override
    {
        f << "[size:2]Material template[/size]\n \n";

        if (auto temp = m_template.resource())
        {
            auto params = temp->parameters();
            std::sort(params.begin(), params.end(), [](const auto& a, const auto& b)
                {
                    if (a->queryType() != b->queryType())
                        return (int)a->queryType() < (int)b->queryType();
                    return a->name().view() < b->name().view();
                });

            f.appendf("{} parameters:\n", params.size());

            const auto* ptr = params.typedData();
            const auto* ptrEnd = ptr + params.size();
            while (ptr < ptrEnd)
            {
                const auto* start = ptr++;
                while (ptr < ptrEnd)
                {
                    if (ptr->get()->queryType() != start->get()->queryType())
                        break;
                    ++ptr;
                }

                char* paramType = "";
                switch (start->get()->queryType())
                {
                    case MaterialDataLayoutParameterType::StaticBool: paramType = "StaticBool"; break;
                    case MaterialDataLayoutParameterType::Float: paramType = "Float"; break;
                    case MaterialDataLayoutParameterType::Vector2: paramType = "Vector2"; break;
                    case MaterialDataLayoutParameterType::Vector3: paramType = "Vector3"; break;
                    case MaterialDataLayoutParameterType::Vector4: paramType = "Vector4"; break;
                    case MaterialDataLayoutParameterType::Color: paramType = "Color"; break;
                    case MaterialDataLayoutParameterType::Texture2D: paramType = "Texture2D"; break;
                }

                f.appendf("[size:1]{} [b]{}[/b] parameters:[/size]\n", (int)(ptr - start), paramType);
                while (start < ptr)
                {
                    f.appendf("{}\n", start->get()->name());
                    ++start;
                }

                f.appendf(" \n");
            }
        }
    }

private:
    MaterialTemplateRef m_template;
};

//--

class MeshStructore_MaterialBase : public IMeshStructureNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshStructore_MaterialBase, IMeshStructureNode);

public:
    MeshStructore_MaterialBase(Mesh* mesh, const MaterialRef& base)
        : IMeshStructureNode(mesh, TempString("[img:file] Base: {}", base.resource()->loadPath().view().fileStem()))
        , m_base(base)
    {}

    virtual void printDetails(IFormatStream& f) const override
    {
        f << "[size:2]Instanced parameters[/size]\n \n";
    }

private:
    MaterialRef m_base;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshStructore_MaterialBase);
    RTTI_PROPERTY(m_base).editable().readonly();
RTTI_END_TYPE();

//--

class MeshStructore_MaterialTexture : public IMeshStructureNode
{
public:
    MeshStructore_MaterialTexture(Mesh* mesh, StringID binding, const ITexture* tex)
        : IMeshStructureNode(mesh, TempString("[img:image] {}: {}", binding, tex ? tex->loadPath().view().fileStem() : "<none>"))
        , m_texture(AddRef(tex))
    {}

    virtual void printDetails(IFormatStream& f) const override
    {
        f << "[size:2]Texture[/size]\n \n";

        if (m_texture)
        {
            const auto& info = m_texture->info();
            f.appendf("Size: [b]{}[/b]x[b]{}[/b]\n", info.width, info.height);
            f.appendf("Format: [b]{}[/b]\n", info.format);
            f.appendf("ColorSpace: [b]{}[/b]\n", info.colorSpace);
            //f.appendf("DataSize: {}\n", MemSize(info.
        }
    }

private:
    StringID m_name;
    TexturePtr m_texture;
};

class MeshStructore_MaterialNode : public IMeshStructureNode
{
public:
    MeshStructore_MaterialNode(Mesh* mesh, StringID name, MaterialInstancePtr data, uint32_t index)
        : IMeshStructureNode(mesh, TempString("[img:material] {}", name))
        , m_name(name)
        , m_index(index)
        , m_data(data)
    {}

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    static MaterialTemplateRef ResolveTemplate(const MaterialRef& ref)
    {
        if (ref.empty())
            return MaterialTemplateRef();

        if (auto temp = rtti_cast<MaterialTemplate>(ref.resource()))
            return MaterialTemplateRef(ref.id(), temp);

        if (auto mi = rtti_cast<MaterialInstance>(ref.resource()))
            return ResolveTemplate(mi->baseMaterial());

        return nullptr;
    }

    virtual void handleItemExpand() override
    {
        if (m_data)
        {
            if (auto shader = ResolveTemplate(m_data->baseMaterial()))
            {
                auto node = RefNew<MeshStructore_MaterialShader>(m_mesh, shader);
                addChild(node);
            }

            if (auto base = m_data->baseMaterial())
            {
                if (base.resource())
                {
                    auto node = RefNew<MeshStructore_MaterialBase>(m_mesh, base);
                    addChild(node);
                }
            }

            if (auto temp = m_data->resolveTemplate())
            {
                for (const auto& param : temp->parameters())
                {
                    if (param->queryType() == MaterialDataLayoutParameterType::Texture2D)
                    {
                        TextureRef currentTexture;
                        if (m_data->readParameterTyped(param->name(), currentTexture))
                        {
                            TextureRef baseTexture;
                            if (temp->readParameterTyped(param->name(), baseTexture))
                            {
                                if (currentTexture != baseTexture)
                                {
                                    auto node = RefNew<MeshStructore_MaterialTexture>(m_mesh, param->name(), currentTexture.resource());
                                    addChild(node);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    virtual void printDetails(IFormatStream& f) const override
    {
        f.appendf("[size:2][b]Material '{}'[/b][/size][br] [br]", m_name);

        uint32_t numTriangles = 0;
        uint32_t numChunks = 0;
        for (const auto& chunk : m_mesh->chunks())
        {
            if (chunk.materialIndex == m_index)
            {
                numChunks += 1;
                numTriangles += chunk.indexCount / 3;
            }
        }

        f.appendf("Number of chunks: [b]{}[/b]\n", numChunks);
        f.appendf("Number of triangles: [b]{}[/b]\n", numTriangles);
    }

private:
    uint32_t m_index = 0;
    StringID m_name;
    MaterialInstancePtr m_data;
};

class MeshStructure_MaterialsRoot : public IMeshStructureNode
{
public:
    MeshStructure_MaterialsRoot(Mesh* mesh)
        : IMeshStructureNode(mesh, "[img:color_swatch] Materials")
    {}

    virtual bool handleItemCanExpand() const override
    {
        return !m_mesh->materials().empty();
    }

    virtual void handleItemExpand() override
    {
        auto materials = m_mesh->materials();
        std::sort(materials.begin(), materials.end(), [](const auto& a, const auto& b) { return a.name.view() < b.name.view(); });

        
        for (auto i : materials.indexRange())
        {
            const auto& mat = materials[i];
            auto node = RefNew<MeshStructore_MaterialNode>(m_mesh, mat.name, mat.material, i);
            addChild(node);
        }
    }

    virtual void printDetails(IFormatStream& f) const override
    {
        f.append("[size:2][b]Material list[/b][/size][br] [br]");
        f.appendf("[b]{}[/b] materials in mesh", m_mesh->materials().size());
    }
};

//--

class MeshStructure_BoxBounds : public IMeshStructureNode
{
public:
    MeshStructure_BoxBounds(Mesh* mesh)
        : IMeshStructureNode(mesh, "[img:wireframe] Bounds")
        , m_box(mesh->bounds())
    {}

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }

    virtual void printDetails(IFormatStream& f) const override
    {
        f.append("[size:2][b]Bounding box[/b][/size][br] [br]");
        f.appendf("Min AABB: [b]{}[/b]\n", m_box.min);
        f.appendf("Max AABB: [b]{}[/b]\n", m_box.max);
    }

private:
    Box m_box;
};

//--

class MeshStructure_GeometryChunk : public IMeshStructureNode
{
public:
    MeshStructure_GeometryChunk(Mesh* mesh, uint32_t index)
        : IMeshStructureNode(mesh, TempString("[img:cylinder] Chunk {}", index))
        , m_index(index)
    {
    }

    virtual void setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {
        settings.forceChunk = m_index;
    }

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }

    static void SizePrint(IFormatStream& s, float size)
    {
        if (size >= 1.0f)
            s.appendPreciseNumber(size / 1.0f, 2).append(" m");
        else if (size >= 0.01f)
            s.appendPreciseNumber(size * 100.0f, 1).append(" cm");
        else if (size >= 0.001f)
            s.appendPreciseNumber(size * 1000.0f, 1).append(" mm");
        else if (size >= 0.000001f)
            s.appendPreciseNumber(size * 1000000.0f, 1).append(" um");
        else
            s.appendPreciseNumber(size * 1000000000.0f, 1).append(" nm");
    }

    static void SizeDensityPrint(IFormatStream& s, float size)
    {
        if (size >= 0.001f)
            s.appendPreciseNumber(1.0f / size, 2).append(" points per m");
        else if (size >= 0.000001f)
            s.appendPreciseNumber(0.001f / size, 1).append(" points per mm");
        else if (size >= 0)
            s.appendPreciseNumber(0.000001f / size, 1).append(" points per um");
    }

    virtual void printDetails(IFormatStream& f) const override
    {
        f.appendf("[size:2][b]Chunk {}[/b][/size][br] [br]", m_index);

        const auto& chunk = m_mesh->chunks()[m_index];
        f.appendf("Vertex format: {}\n", chunk.vertexFormat);
        f.appendf("Quantization X: ");
        SizePrint(f, chunk.quantizationScale.x);
        f.appendf(" (");
        SizeDensityPrint(f, chunk.quantizationScale.x);
        f.appendf(")\n");
        f.appendf("Quantization Y: ");
        SizePrint(f, chunk.quantizationScale.y);
        f.appendf(" (");
        SizeDensityPrint(f, chunk.quantizationScale.y);
        f.appendf(")\n");
        f.appendf("Quantization Z: ");
        SizePrint(f, chunk.quantizationScale.z);
        f.appendf(" (");
        SizeDensityPrint(f, chunk.quantizationScale.z);
        f.appendf(")\n");

        f.appendf("Detail levels: [color:#8FF]");
        for (auto i : m_mesh->detailLevels().indexRange())
            if (chunk.detailMask & (1U << i))
                f.appendf("LOD{} ", i);
        f.append("[/color]\n");

        f.append("Render mask: [color:#8F8]");
        auto mask = (MeshChunkRenderingMask)chunk.renderMask;
        if (mask.test(MeshChunkRenderingMaskBit::Scene)) f << "Scene ";
        if (mask.test(MeshChunkRenderingMaskBit::ObjectShadows)) f << "ObjectShadows ";
        if (mask.test(MeshChunkRenderingMaskBit::LocalShadows)) f << "LocalShadows ";
        if (mask.test(MeshChunkRenderingMaskBit::Cascade0)) f << "Cascade0 ";
        if (mask.test(MeshChunkRenderingMaskBit::Cascade1)) f << "Cascade1 ";
        if (mask.test(MeshChunkRenderingMaskBit::Cascade2)) f << "Cascade2 ";
        if (mask.test(MeshChunkRenderingMaskBit::Cascade3)) f << "Cascade3 ";
        if (mask.test(MeshChunkRenderingMaskBit::LodMerge)) f << "LodMerge ";
        if (mask.test(MeshChunkRenderingMaskBit::ShadowMesh)) f << "ShadowMesh ";
        if (mask.test(MeshChunkRenderingMaskBit::LocalReflection)) f << "LocalReflection ";
        if (mask.test(MeshChunkRenderingMaskBit::GlobalReflection)) f << "GlobalReflection ";
        if (mask.test(MeshChunkRenderingMaskBit::Lighting)) f << "Lighting ";
        if (mask.test(MeshChunkRenderingMaskBit::StaticOccluder)) f << "StaticOccluder ";
        if (mask.test(MeshChunkRenderingMaskBit::DynamicOccluder)) f << "DynamicOccluder ";
        if (mask.test(MeshChunkRenderingMaskBit::ConvexCollision)) f << "ConvexCollision ";
        if (mask.test(MeshChunkRenderingMaskBit::ExactCollision)) f << "ExactCollision ";
        if (mask.test(MeshChunkRenderingMaskBit::Cloth)) f << "Cloth ";
        f.append("[/color]\n \n");

        f.append("[size:2]Index data[/size]\n");
        f.appendf("Num indices: [b]{}[/b]\n", chunk.indexCount);
        f.appendf("GPU index data size: [b]{}[/b]\n", MemSize(chunk.unpackedIndexSize));
        f.appendf("Disk index data size: [b]{}[/b]\n", MemSize(chunk.packedIndexData.compressedSize()));
        f.appendf("Index data size compression: [b]{}[/b]\n", Prec(chunk.unpackedIndexSize ? chunk.packedIndexData.compressedSize() / (double)chunk.unpackedIndexSize : 1.0f, 2));
        f.append(" \n");

        f.append("[size:2]Vertex data[/size]\n");
        f.appendf("Num vertices: [b]{}[/b]\n", chunk.vertexCount);
        f.appendf("GPU index data size: [b]{}[/b]\n", MemSize(chunk.unpackedVertexSize));
        f.appendf("Disk index data size: [b]{}[/b]\n", MemSize(chunk.packedVertexData.compressedSize()));
        f.appendf("Index data size compression: [b]{}[/b]\n", Prec(chunk.unpackedVertexSize ? chunk.packedVertexData.compressedSize() / (double)chunk.unpackedVertexSize : 1.0f, 2));
    }

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    virtual void handleItemExpand() override
    {
        const auto& chunk = m_mesh->chunks()[m_index];
        const auto& mat = m_mesh->materials()[chunk.materialIndex];

        auto node = RefNew<MeshStructore_MaterialNode>(m_mesh, mat.name, mat.material, chunk.materialIndex);
        addChild(node);
    }

private:
    uint32_t m_index;
};

class MeshStructure_GeometryDetail : public IMeshStructureNode
{
public:
    MeshStructure_GeometryDetail(Mesh* mesh, uint32_t index)
        : IMeshStructureNode(mesh, TempString("[img:shapes] LOD {}", index))
        , m_index(index)
    {
    }

    virtual void printDetails(IFormatStream& f) const override
    {
        f.appendf("[size:2][b]LOD {}[/b][/size][br] [br]", m_index);

        const auto& lod = m_mesh->detailLevels()[m_index];
        f.appendf("Show distance: [b]{}[/b]\n", lod.rangeMin);
        f.appendf("Hide distance: [b]{}[/b]\n", lod.rangeMax);

        uint32_t numChunks = 0;
        uint32_t totalIndexCount = 0;
        uint32_t totalVertexCount = 0;
        uint32_t totalIndexPackedSize = 0;
        uint32_t totalVertexPackedSize = 0;
        uint32_t totalIndexUnpackedSize = 0;
        uint32_t totalVertexUnpackedSize = 0;
        HashSet<uint32_t> materialIndices;
        const auto mask = 1U << m_index;
        for (auto i : m_mesh->chunks().indexRange())
        {
            const auto& chunk = m_mesh->chunks()[i];
            if (chunk.detailMask & mask)
            {
                materialIndices.insert(chunk.materialIndex);

                numChunks += 1;
                totalIndexCount += chunk.indexCount;
                totalVertexCount += chunk.vertexCount;
                totalIndexPackedSize += chunk.packedIndexData.compressedSize();
                totalVertexPackedSize += chunk.packedVertexData.compressedSize();
                totalIndexUnpackedSize += chunk.unpackedIndexSize;
                totalVertexUnpackedSize += chunk.unpackedVertexSize;
            }
        }

        f.appendf("Num unique chunks: {}\n", numChunks);
        f.appendf("Num unique materials: {}\n \n", materialIndices.size());

        f.append("[size:2]Total index data[/size]\n");
        f.appendf("Num indices: [b]{}[/b]\n", totalIndexCount);
        f.appendf("GPU index data size: [b]{}[/b]\n", MemSize(totalIndexUnpackedSize));
        f.appendf("Disk index data size: [b]{}[/b]\n", MemSize(totalIndexPackedSize));
        f.appendf("Index data size compression: [b]{}[/b]\n", Prec(totalIndexUnpackedSize ? totalIndexPackedSize / (double)totalIndexUnpackedSize : 1.0f, 2));
        f.append(" \n");

        f.append("[size:2]Total vertex data[/size]\n");
        f.appendf("Num vertices: [b]{}[/b]\n", totalVertexCount);
        f.appendf("GPU vertex data size: [b]{}[/b]\n", MemSize(totalVertexUnpackedSize));
        f.appendf("Disk vertex data size: [b]{}[/b]\n", MemSize(totalVertexPackedSize));
        f.appendf("Vertex data size compression: [b]{}[/b]\n", Prec(totalVertexUnpackedSize ? totalVertexPackedSize / (double)totalVertexUnpackedSize : 1.0f, 2));
    }

    virtual void setup(MeshPreviewPanelSettings& settings, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {
        settings.forceLod = m_index;
    }

    virtual void render(rendering::FrameParams& frame, const Array<const IMeshStructureNode*>& allNodes, const IMeshStructureNode* topNode) const override
    {

    }

    virtual bool handleItemCanExpand() const override
    {
        return true;
    }

    virtual void handleItemExpand() override
    {
        const auto mask = 1U << m_index;

        for (auto i : m_mesh->chunks().indexRange())
        {
            const auto& chunk = m_mesh->chunks()[i];
            if (chunk.detailMask & mask)
            {
                auto node = RefNew<MeshStructure_GeometryChunk>(m_mesh, i);
                addChild(node);
            }
        }
    }

private:
    uint32_t m_index = 0;
};

//--

void CreateStructure(ui::TreeViewEx* view, Mesh* mesh)
{
    {
        auto node = RefNew<MeshStructure_BoxBounds>(mesh);
        view->addRoot(node);
    }

    {
        auto node = RefNew<MeshStructure_MaterialsRoot>(mesh);
        view->addRoot(node);
    }

    for (auto i : mesh->detailLevels().indexRange())
    {
        auto node = RefNew<MeshStructure_GeometryDetail>(mesh, i);
        view->addRoot(node);
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
