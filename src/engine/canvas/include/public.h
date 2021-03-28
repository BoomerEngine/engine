/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_canvas_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

/// polygon winding
enum class CanvasWinding : uint8_t
{
    CCW = 1, // CanvasWinding for solid shapes
    CW = 2, // CanvasWinding for holes
};

//--

/// raster composite operation - determines how pixels are mixed
/// implemented using classical blending scheme
enum class CanvasBlendOp : uint8_t
{
	Copy, // blending disabled, NOTE: scissor done using discard only
	AlphaPremultiplied, // src*1 + dest*(1-srcAlpha)
	AlphaBlend, // LEGACY ONLY: src*srcAlpha + dest*(1-srcAlpha)
	Addtive, // LEGACY ONLY: src*1 + dest*1

	MAX,
};

//--

struct CanvasRenderStyle;

struct CanvasGeometry;
class CanvasGeometryBuilder;

class Canvas;
class CanvasService;
class CanvasRenderer;

class ICanvasBatchRenderer;

//--

/// icon for use with canvas
class ENGINE_CANVAS_API CanvasImage : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasImage, IObject);

public:
	CanvasImage(const Image* data, bool wrapU=false, bool wrapV=false);
	CanvasImage(StringView depotPath, bool wrapU = false, bool wrapV = false);
    virtual ~CanvasImage();

    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }

    INLINE AtlasImageID id() const { return m_id; }

	INLINE bool wrapU() const { return m_wrapU; }
	INLINE bool wrapV() const { return m_wrapV; }

private:
    AtlasImageID m_id = 0;

    uint32_t m_width = 1;
    uint32_t m_height = 1;

	bool m_wrapU = false;
	bool m_wrapV = false;

    DynamicImageAtlasEntryPtr m_entry;
};

typedef RefPtr<CanvasImage> CanvasImagePtr;

//--

/// type of geometry path in canvas rendering
enum class CanvasBatchType : uint8_t
{
	FillConvex, // convex polygon fill, no stencil, just fill
	FillConcave, // concave polygon fill, previously masked with FillConcave, contains 4 corner vertices
	ConcaveMask, // stencil masking for concave fill (ends with FillConcave)
};

/// vertices in batch
enum class CanvasBatchPacking : uint8_t
{
	TriangleList,
	TriangleFan,
	TriangleStrip,
	Quads,
};

//--

#pragma pack(push)
#pragma pack(1)
struct CanvasVertex
{
	static const uint8_t MASK_FILL = 1; // we are a fill
	static const uint8_t MASK_STROKE = 2; // we are a stroke
	static const uint8_t MASK_GLYPH = 4; // we are a glyph
	static const uint8_t MASK_HAS_IMAGE = 8; // we have image
	static const uint8_t MASK_HAS_WRAP_U = 16; // do U wrapping
	static const uint8_t MASK_HAS_WRAP_V = 32; // do V wrapping
	static const uint8_t MASK_HAS_FRINGE = 64; // we have an extra fringe extrusion
	static const uint8_t MASK_IS_CONVEX = 128; // what we are rendering is convex

	Vector2 pos; // transformed original geometry vertex
	Vector2 uv; // original uv
	Vector2 clipUV; // UV from the scissor, calculated when placing vertices
	Color color; // original color

	uint16_t attributeIndex = 0; // attributes table entry
	uint16_t attributeFlags = 0; // cached styling information in form of flags
	uint16_t imageEntryIndex = 0; // entry in the image table
	uint16_t imagePageIndex = 0; // page index of the image
};
#pragma pack(pop)

	
//--

struct CanvasBatch
{
	uint32_t vertexOffset = 0;
	uint32_t vertexCount = 0;

	CanvasBatchType type = CanvasBatchType::FillConvex;
	CanvasBatchPacking packing = CanvasBatchPacking::TriangleList;
	CanvasBlendOp op = CanvasBlendOp::AlphaPremultiplied;

	uint64_t glyphPageMask = 0;

	uint16_t renderDataOffset = 0;
	uint8_t renderDataSize = 0;
	uint8_t rendererIndex = 0;
};

//--

END_BOOMER_NAMESPACE()
