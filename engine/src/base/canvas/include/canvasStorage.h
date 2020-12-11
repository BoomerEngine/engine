/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

namespace base
{
    namespace canvas
    {
		//--

		/// cached geometry data
		/// NOTE: baked geometry is only valid when used with the same storage that generated it
		/// NOTE: baked geometry is only valid if atlas storage was NOT reset/reorganized in the mean time
		class BASE_CANVAS_API BakedGeometry : public IReferencable
		{
			RTTI_DECLARE_POOL(POOL_CANVAS);

		public:
			BakedGeometry(base::Array<Vertex>&& vertices, base::Array<Batch>&& outBatches, const Vector2& minBounds, const Vector2& maxBounds);
			virtual ~BakedGeometry();

			INLINE const base::Array<Vertex>& vertices() const { return m_vertices; }
			INLINE const base::Array<Batch>& batches() const { return m_batches; }

			INLINE const Vector2& vertexBoundsMin() const { return m_vertexBoundsMin; }
			INLINE const Vector2& vertexBoundsMax() const { return m_vertexBoundsMax; }

		protected:
			base::Array<Vertex> m_vertices;
			base::Array<Batch> m_batches;

			Vector2 m_vertexBoundsMin;
			Vector2 m_vertexBoundsMax;
		};

		//--

		struct ImageAtlasEntryInfo
		{
			Vector2 uvOffset;
			Vector2 uvScale;
			Vector2 uvMax;
			char pageIndex = -1; // -1 - not placed
			bool wrap = false;
		};

		//--

		/// abstract canvas renderer, used to push placed geometries to some actual rendering target
		class BASE_CANVAS_API IStorage : public IReferencable
		{
			RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IStorage);
			RTTI_DECLARE_POOL(POOL_CANVAS);

		public:
			IStorage();
			virtual ~IStorage();

			//--

			// perform housekeeping - especially check if atlases run out of storage
			// returns "true" if rebuild of any internal textures was required which signals that ALL cached geometries are now invalid and should be regenerated
			virtual void conditionalRebuild(bool* outAtlasesWereRebuilt = nullptr) = 0;

			//--

			// create image atlas that can be used to hold to to images usable in rendering within this renderer
			// NOTE: it's good idea to keep images of different sorts in separate atlases, e.g:
			//  - ui icons are mostly static so they go to one atlas
			//  - resource thumbnails are dynamic and they are stored in other one, etc.
			virtual ImageAtlasIndex createAtlas(uint32_t pageSize, uint32_t numPages, StringView debugName = "") = 0;

			// destroy previously created atlas
			virtual void destroyAtlas(ImageAtlasIndex atlas) = 0;

			//--

			// place image in the atlas, this might fail if atlas is invalid or we run out of space in it
			virtual ImageEntry registerImage(ImageAtlasIndex atlasIndex, const image::Image* ptr, bool supportWrapping = false, int additionalPixelBorder = 0) = 0;

			// remove image from the atlas
			virtual void unregisterImage(ImageEntry entry) = 0;

			//--

			// find placement of given image in the atlas
			virtual const ImageAtlasEntryInfo* findRenderDataForAtlasEntry(const ImageEntry& entry) const = 0;

			// find placement of given font glyph
			virtual const ImageAtlasEntryInfo* findRenderDataForGlyph(const font::Glyph* glyph) const = 0;

			//--
        };

		//--

    } // canvas
} // base

