/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_canvas_glue.inl"

namespace base
{
    namespace canvas
    {
		//--

        /// polygon winding
        enum class Winding : uint8_t
        {
            CCW = 1, // Winding for solid shapes
            CW = 2, // Winding for holes
        };

		//--

		/// raster composite operation - determines how pixels are mixed
		/// implemented using classical blending scheme
		enum class BlendOp : uint8_t
		{
			Copy, // blending disabled, NOTE: scissor done using discard only
			AlphaPremultiplied, // src*1 + dest*(1-srcAlpha)
			AlphaBlend, // LEGACY ONLY: src*srcAlpha + dest*(1-srcAlpha)
			Addtive, // LEGACY ONLY: src*1 + dest*1

			MAX,
		};

		//--

        struct RenderStyle;

        struct Geometry;
        class GeometryBuilder;

		class Canvas;

		class IStorage;

		struct ImageAtlasEntryInfo;

		//--

		typedef uint16_t ImageAtlasIndex;
		typedef uint16_t ImageEntryIndex;

		struct BASE_CANVAS_API ImageEntry
		{
			ImageAtlasIndex atlasIndex = 0; // index of the atlas
			ImageEntryIndex entryIndex = 0; // index of entry in the atlas
			uint16_t width = 0; // width of the registered image
			uint16_t height = 0; // height of the registered image

			INLINE ImageEntry() {};
			INLINE operator bool() const { return atlasIndex != 0; }

			INLINE bool operator==(const ImageEntry& other) const { return atlasIndex == other.atlasIndex && entryIndex == other.entryIndex; }
			INLINE bool operator!=(const ImageEntry& other) const { return !operator==(other); }

			static uint32_t CalcHash(const ImageEntry& entry);
		};

		//--

		/// type of geometry path in canvas rendering
		enum class BatchType : uint8_t
		{
			FillConvex, // convex polygon fill, no stencil, just fill
			FillConcave, // concave polygon fill, previously masked with FillConcave, contains 4 corner vertices
			ConcaveMask, // stencil masking for concave fill (ends with FillConcave)
		};

		/// vertices in batch
		enum class BatchPacking : uint8_t
		{
			TriangleList,
			TriangleFan,
			TriangleStrip,
			Quads,
		};

		//--

#pragma pack(push)
#pragma pack(1)
		struct Vertex
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

		struct Batch
		{
			uint32_t vertexOffset = 0;
			uint32_t vertexCount = 0;

			BatchType type = BatchType::FillConvex;
			BatchPacking packing = BatchPacking::TriangleList;
			BlendOp op = BlendOp::AlphaPremultiplied;
			uint8_t atlasIndex = 0;

			uint16_t renderDataOffset = 0;
			uint8_t renderDataSize = 0;
			uint8_t rendererIndex = 0;
		};

		//--

    } // canvas
} // base