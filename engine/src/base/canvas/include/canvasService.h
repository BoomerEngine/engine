/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/image/include/imageView.h"

namespace base
{
    namespace canvas
    {
		//--

		/// canvas atlas texture updater
		class BASE_CANVAS_API ICanvasAtlasSync : public NoCopy
		{
		public:
			virtual ~ICanvasAtlasSync();

			struct PageUpdate
			{
				uint8_t pageIndex = 0;
				image::ImageView data;
				Rect rect;
			};

			struct EntryUpdate
			{
				uint32_t firstEntry = 0;
				uint32_t numEntries = 0;
				
				const ImageAtlasEntryInfo* data = nullptr;
			};

			struct AtlasUpdate
			{
				uint32_t size = 0;
				uint32_t numPages = 0;

				InplaceArray<PageUpdate, 4> pageUpdate;
				EntryUpdate entryUpdate;				
			};

			// flush update to glyph cache
			virtual void updateGlyphCache(rendering::command::CommandWriter& cmd, const AtlasUpdate& update) = 0;

			// flush updates to atlas entries
			virtual void updateAtlas(rendering::command::CommandWriter& cmd, ImageAtlasIndex atlasIndex, const AtlasUpdate& update) = 0;
		};

		//--

		/// canvas data/rendering service
		class BASE_CANVAS_API CanvasService : public app::ILocalService
		{
			RTTI_DECLARE_VIRTUAL_CLASS(CanvasService, app::ILocalService);
			RTTI_DECLARE_POOL(POOL_CANVAS);

		public:
			CanvasService();

			//--

			// find placement of given image in the atlas
			// NOTE: atlas must be registered first
			const ImageAtlasEntryInfo* findRenderDataForAtlasEntry(const ImageEntry& entry) const;

			// find placement of given font glyph
			const ImageAtlasEntryInfo* findRenderDataForGlyph(const font::Glyph* glyph) const;

			//--

			// sync asset changes
			void syncAssetChanges(rendering::command::CommandWriter& cmd, uint64_t atlasMask, uint64_t glpyhMask, ICanvasAtlasSync& sync);

			//--

		private:
			static const uint32_t MAX_ATLASES = 64;

			virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
			virtual void onShutdownService() override final;
			virtual void onSyncUpdate() override final;

			IAtlas* m_atlasRegistry[MAX_ATLASES];

			GlyphCache* m_glyphCache = nullptr;

			bool registerAtlas(IAtlas* atlas, ImageAtlasIndex& outIndex);
			void unregisterAtlas(IAtlas* atlas, ImageAtlasIndex index);

			friend class IAtlas;
		};

		//--

    } // canvas
} // base

