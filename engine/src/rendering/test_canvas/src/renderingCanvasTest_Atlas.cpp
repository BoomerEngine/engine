/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"
#include "base/math/include/randomMersenne.h"
#include "base/resources/include/public.h"
#include "base/system/include/timing.h"

namespace rendering
{
    namespace test
    {
        struct SimpleAtlasEntry
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SimpleAtlasEntry);

            base::StringID name;
            uint32_t x=0, y=0, w=0, h=0;
            base::Rect rect;
        };

        RTTI_BEGIN_TYPE_CLASS(SimpleAtlasEntry);
            RTTI_PROPERTY(name);
            RTTI_PROPERTY(x);
            RTTI_PROPERTY(y);
            RTTI_PROPERTY(w);
            RTTI_PROPERTY(h);
        RTTI_END_TYPE();

        class SimpleAtlas : public base::res::ITextResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SimpleAtlas, base::res::ITextResource);

        public:
            base::Array<SimpleAtlasEntry> m_entries;

            base::HashMap<base::StringID, const SimpleAtlasEntry*> m_entriesMap;
            SimpleAtlasEntry m_emptyEntry;

            const SimpleAtlasEntry* findEntry(base::StringID name) const
            {
                const SimpleAtlasEntry* ret = nullptr;
                if (m_entriesMap.find(name, ret))
                    return ret;
                return &m_emptyEntry;
            }

            virtual void onPostLoad() override
            {
                TBaseClass::onPostLoad();

                m_entriesMap.reserve(m_entries.size());
                for (auto& entry : m_entries)
                {
                    m_entriesMap[entry.name] = &entry;
                    entry.rect.min.x = entry.x;
                    entry.rect.min.y = entry.y;
                    entry.rect.max.x = entry.x + entry.w;
                    entry.rect.max.y = entry.y + entry.h;
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SimpleAtlas);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4atlas");
            RTTI_PROPERTY(m_entries);
        RTTI_END_TYPE();

        //--


        typedef base::RefPtr<base::canvas::Geometry> GeometryPtr;

        class GameAtlasTiles : public base::NoCopy
        {
        public:
            GameAtlasTiles(const SimpleAtlas& atlas, const base::image::ImagePtr& img)
                : m_atlas(atlas)
                , m_image(img)
            {
                m_grass[0] = MakeImageStyle("GrassFull0"_id);
                m_grass[1] = MakeImageStyle("GrassFull1"_id);
                m_grass[2] = MakeImageStyle("GrassFull2"_id);
                m_grass[3] = MakeImageStyle("GrassFull3"_id);
                
                m_lightDirt.m_outer[0][0] = MakeImageStyle("LightGroundTL"_id);
                m_lightDirt.m_outer[1][0] = MakeImageStyle("LightGroundTC"_id);
                m_lightDirt.m_outer[2][0] = MakeImageStyle("LightGroundTR"_id);
                m_lightDirt.m_outer[0][1] = MakeImageStyle("LightGroundCL"_id);
                m_lightDirt.m_outer[1][1] = MakeImageStyle("LightGroundCC"_id);
                m_lightDirt.m_outer[2][1] = MakeImageStyle("LightGroundCR"_id);
                m_lightDirt.m_outer[0][2] = MakeImageStyle("LightGroundBL"_id);
                m_lightDirt.m_outer[1][2] = MakeImageStyle("LightGroundBC"_id);
                m_lightDirt.m_outer[2][2] = MakeImageStyle("LightGroundBR"_id);
                m_lightDirt.m_inner[0][0] = MakeImageStyle("LightGroundInnerTL"_id);
                m_lightDirt.m_inner[1][0] = MakeImageStyle("LightGroundInnerTR"_id);
                m_lightDirt.m_inner[0][1] = MakeImageStyle("LightGroundInnerBL"_id);
                m_lightDirt.m_inner[1][1] = MakeImageStyle("LightGroundInnerBR"_id);

                m_darkDirt.m_outer[0][0] = MakeImageStyle("DarkGroundTL"_id);
                m_darkDirt.m_outer[1][0] = MakeImageStyle("DarkGroundTC"_id);
                m_darkDirt.m_outer[2][0] = MakeImageStyle("DarkGroundTR"_id);
                m_darkDirt.m_outer[0][1] = MakeImageStyle("DarkGroundCL"_id);
                m_darkDirt.m_outer[1][1] = MakeImageStyle("DarkGroundCC"_id);
                m_darkDirt.m_outer[2][1] = MakeImageStyle("DarkGroundCR"_id);
                m_darkDirt.m_outer[0][2] = MakeImageStyle("DarkGroundBL"_id);
                m_darkDirt.m_outer[1][2] = MakeImageStyle("DarkGroundBC"_id);
                m_darkDirt.m_outer[2][2] = MakeImageStyle("DarkGroundBR"_id);

                m_darkDirt.m_inner[0][0] = MakeImageStyle("DarkGroundInnerTL"_id);
                m_darkDirt.m_inner[1][0] = MakeImageStyle("DarkGroundInnerTR"_id);
                m_darkDirt.m_inner[0][1] = MakeImageStyle("DarkGroundInnerBL"_id);
                m_darkDirt.m_inner[1][1] = MakeImageStyle("DarkGroundInnerBR"_id);

                m_treeTop = MakeImageStyle("TreeTop01"_id, false);
                m_treeBottom = MakeImageStyle("TreeBottom01"_id, false);

                /*m_grass[0] = base::canvas::ImagePattern(img, base::canvas::ImagePatternSettings());
                m_grass[1] = base::canvas::ImagePattern(img, base::canvas::ImagePatternSettings());
                m_grass[2] = base::canvas::ImagePattern(img, base::canvas::ImagePatternSettings());*/
            }

            static const uint32_t TILE_SIZE = 32;

            base::canvas::RenderStyle m_grass[4];

            struct Patch
            {
                base::canvas::RenderStyle m_outer[3][3];
                base::canvas::RenderStyle m_inner[2][2];
            };

            Patch m_darkDirt;
            Patch m_lightDirt;

            base::canvas::RenderStyle m_treeTop;
            base::canvas::RenderStyle m_treeBottom;
            
            GeometryPtr generateTree() const
            {
                base::canvas::GeometryBuilder b;

                {
                    b.resetTransform();
                    b.translatei(-48, -64);
                    b.fillPaint(m_treeBottom);
                    b.beginPath();
                    b.rect(0, 0, 96, 64);
                    b.fill();
                }

                {
                    b.resetTransform();
                    b.translatei(-48, -(48+96));
                    b.fillPaint(m_treeTop);
                    b.beginPath();
                    b.rect(0, 0, 96, 96);
                    b.fill();
                }

                auto ret = base::CreateSharedPtr<base::canvas::Geometry>();
                b.compositeOperation(base::canvas::CompositeOperation::SourceOver);
                b.extract(*ret);
                return ret;
            }

            GeometryPtr generateSimpleObject(base::StringID id) const
            {
                auto style = MakeImageStyle(id, false);

                base::canvas::GeometryBuilder b;

                {
                    b.resetTransform();
                    b.translatei(-(int)(style.image->width() / 2), -(int)(style.image->height()));
                    b.fillPaint(style);
                    b.beginPath();
                    b.rect(0, 0, style.image->width(), style.image->height());
                    b.fill();
                }

                auto ret = base::CreateSharedPtr<base::canvas::Geometry>();
                b.compositeOperation(base::canvas::CompositeOperation::SourceOver);
                b.extract(*ret);
                return ret;
            }

            GeometryPtr generateGrassPatch(base::MTGenerator& rnd, uint32_t width, uint32_t height) const
            {
                base::canvas::GeometryBuilder b;
                b.compositeOperation(base::canvas::CompositeOperation::Copy);

                for (uint32_t y = 0; y < height; ++y)
                {
                    for (uint32_t x = 0; x < width; ++x)
                    {
                        b.resetTransform();
                        b.translatei(x * TILE_SIZE, y * TILE_SIZE);

                        auto grassType = 0;

                        auto pp = rnd.nextUint32() % 16;
                        if (pp > 7)
                            grassType = 1;
                        if (pp > 11)
                            grassType = 2;
                        if (pp > 14)
                            grassType = 3;

                        b.fillPaint(m_grass[grassType]);

                        b.beginPath();
                        b.rect(0, 0, TILE_SIZE, TILE_SIZE);
                        b.fill();
                    }
                }

                auto ret = base::CreateSharedPtr<base::canvas::Geometry>();
                b.extract(*ret);
                return ret;
            }

            GeometryPtr generateDirtyPatch(base::MTGenerator& rnd, uint32_t width, uint32_t height, uint32_t circles, uint32_t maxSize, const Patch& styles) const
            {
                base::Array<char> map;
                map.prepareWith(width * height, 0);
                memset(map.data(), 0, map.dataSize());

                for (uint32_t i = 0; i < circles; ++i)
                {
                    uint32_t size = 1 + rnd.nextUint32() % maxSize;
                    int minX = size + 1;
                    int minY = size + 1;
                    int maxX = width - (size + 1);
                    int maxY = height - (size + 1);
                    if (minX < maxX && minY < maxY)
                    {
                        float x = base::Lerp((float)minX, (float)minY, rnd.nextFloat());
                        float y = base::Lerp((float)minY, (float)minY, rnd.nextFloat());

                        minX = (int)std::floor(x - size);
                        minY = (int)std::floor(y - size);
                        maxX = (int)std::ceil(x + size);
                        maxY = (int)std::ceil(y + size);

                        if (minX < 1) minX = 1;
                        if (minY < 1) minY = 1;
                        if (maxX > (width - 2)) maxX = (width - 2);
                        if (maxY > (width - 2)) maxY = (width - 2);

                        for (int py = minY; py <= maxY; ++py)
                        {
                            for (int px = minX; px <= maxX; ++px)
                            {
                                float sq = (px - x) * (px - x) + (py - y) * (py - y);
                                if (sq <= (size * size))
                                {
                                    map[px + py * width] = true;
                                }
                            }
                        }
                    }
                }

                base::canvas::GeometryBuilder b;
                b.compositeOperation(base::canvas::CompositeOperation::SourceOver);
                for (uint32_t y = 1; y < (height-1); ++y)
                {
                    const char* trow = map.typedData() + 1 + ((y - 1) * width);
                    const char* crow = map.typedData() + 1 + ((y + 0) * width);
                    const char* brow = map.typedData() + 1 + ((y + 1) * width);

                    for (uint32_t x = 1; x < (width-1); ++x, ++trow, ++crow, ++brow)
                    {
                        uint16_t code = 0;
                        bool tl = trow[-1];
                        bool tc = trow[0];
                        bool tr = trow[1];
                        bool cl = crow[-1];
                        bool cc = crow[0];
                        bool cr = crow[1];
                        bool bl = brow[-1];
                        bool bc = brow[0];
                        bool br = brow[1];

                        const base::canvas::RenderStyle* style = nullptr;
                        if (cc)
                        {
                            if (tc && bc && cl && cr)
                            {
                                if (!tl && tr && bl && br)
                                    style = &styles.m_inner[1][1];
                                else if (tl && !tr && bl && br)
                                    style = &styles.m_inner[0][1];
                                else if (tl && tr && !bl && br)
                                    style = &styles.m_inner[1][0];
                                else if (tl && tr && bl && !br)
                                    style = &styles.m_inner[0][0];
                                else if (tl && tr && bl && br)
                                    style = &styles.m_outer[1][1];
                                else if (!tl && !tr && bl && br)
                                    style = &styles.m_outer[1][0];
                                else if (tl && tr && !bl && !br)
                                    style = &styles.m_outer[1][2];
                                else if (!tl && tr && !bl && br)
                                    style = &styles.m_outer[0][1];
                                else if (tl && !tr && bl && !br)
                                    style = &styles.m_outer[2][1];
                            }
                            else if (tc && bc && !cl && cr)
                                style = &styles.m_outer[0][1];
                            else if (tc && bc && cl && !cr)
                                style = &styles.m_outer[2][1];
                            else if (!tc && bc && cl && cr)
                                style = &styles.m_outer[1][0];
                            else if (tc && !bc && cl && cr)
                                style = &styles.m_outer[1][2];
                            else if (!tc && bc && !cl && cr)
                                style = &styles.m_outer[0][0];
                            else if (!tc && bc && cl && !cr)
                                style = &styles.m_outer[2][0];
                            else if (tc && !bc && !cl && cr)
                                style = &styles.m_outer[0][2];
                            else if (tc && !bc && cl && !cr)
                                style = &styles.m_outer[2][2];
                        }

                        if (style)
                        {
                            b.resetTransform();
                            b.translatei(x * TILE_SIZE, y * TILE_SIZE);
                            b.fillPaint(*style);
                            b.beginPath();
                            b.rect(0, 0, TILE_SIZE, TILE_SIZE);
                            b.fill();
                        }
                    }
                }

                auto ret = base::CreateSharedPtr<base::canvas::Geometry>();
                b.extract(*ret);
                return ret;
            }

            base::canvas::RenderStyle MakeImageStyle(base::StringID name, bool wrap = true) const
            {
                const auto* entry = m_atlas.findEntry(name);

                const auto image = base::CreateSharedPtr<base::image::Image>(base::image::PixelFormat::Uint8_Norm, 4, entry->rect.width(), entry->rect.height());
                Copy(m_image->view().subView(entry->rect.left(), entry->rect.top(), entry->rect.width(), entry->rect.height()), image->view());

                return ImagePattern(image, base::canvas::ImagePatternSettings().wrapU(wrap).wrapV(wrap));
            }


        private:
            const SimpleAtlas& m_atlas;
            base::image::ImagePtr m_image;

        };

        class GameLevel : public base::NoCopy
        {
        public:
            static const uint32_t TILE_SIZE = 32;
            static const uint32_t PATCH_SIZE = 16;

            GameLevel(uint32_t width, uint32_t height, const GameAtlasTiles& atlas)
                : m_width(width)
                , m_height(height)
                , m_atlas(atlas)
            {
                m_pixelWidth = width * TILE_SIZE;
                m_pixelHeight = height * TILE_SIZE;

                generateLevel();
            }

            void render(base::canvas::Canvas& c) const
            {
                for (const auto& obj : m_grass)
                    obj.render(c);
                for (const auto& obj : m_patches)
                    obj.render(c);
                for (const auto& obj : m_objects)
                    obj.render(c);
            }

            INLINE uint32_t pixelWidth() const { return m_pixelWidth; }
            INLINE uint32_t pixelHeight() const { return m_pixelHeight; }

        private:
            struct Sprite
            {
                GeometryPtr geometry;
                base::Point pos;

                INLINE Sprite() {}; 
                INLINE Sprite(const GeometryPtr& geom, int x, int y) : geometry(geom), pos(x, y) {};

                void render(base::canvas::Canvas& c) const
                {
                    c.placement(pos.x, pos.y);
                    c.place(*geometry);
                }
            };

            base::Array<Sprite> m_grass;
            base::Array<Sprite> m_patches;
            base::Array<Sprite> m_objects;

            const GameAtlasTiles& m_atlas;

            uint32_t m_width = 0;
            uint32_t m_height = 0;

            uint32_t m_pixelWidth = 0;
            uint32_t m_pixelHeight = 0;

            void generateLevel()
            {
                base::MTGenerator rnd;

                // generate ground
                {
                    
                    for (uint32_t y = 0; y < m_width; y += PATCH_SIZE)
                    {
                        for (uint32_t x = 0; x < m_width; x += PATCH_SIZE)
                        {
                            auto geometry = m_atlas.generateGrassPatch(rnd, PATCH_SIZE, PATCH_SIZE);
                            m_grass.emplaceBack(geometry, x * TILE_SIZE, y * TILE_SIZE);
                        }
                    }
                }

                // generate islands
                {
                    uint32_t numIslands = (m_width * m_height) / 100;

                    for (uint32_t i = 0; i < numIslands; ++i)
                    {
                        uint32_t size = 10 + (rnd.nextUint32() % 20);

                        uint32_t x = rnd.nextUint32() % m_width;
                        uint32_t y = rnd.nextUint32() % m_height;

                        if (auto geometry = m_atlas.generateDirtyPatch(rnd, size, size, 1 + size/4, std::max<uint32_t>(4, size / 2), (i & 1) ? m_atlas.m_darkDirt : m_atlas.m_lightDirt))
                            m_patches.emplaceBack(geometry, x * TILE_SIZE, y * TILE_SIZE);
                    }
                }

                // generate objects
                {
                    struct Entry
                    {
                        GeometryPtr geom;
                        uint32_t chance = 0;

                        INLINE Entry() {};
                        INLINE Entry(uint32_t ch, const GeometryPtr& g) : chance(ch), geom(g) {};
                    };

                    base::Array<Entry> geometries;
                    geometries.emplaceBack(20, m_atlas.generateTree());
                    geometries.emplaceBack(3, m_atlas.generateSimpleObject("Head"_id));
                    geometries.emplaceBack(4, m_atlas.generateSimpleObject("Stump"_id));
                    geometries.emplaceBack(1, m_atlas.generateSimpleObject("Shroom"_id));

                    std::sort(geometries.begin(), geometries.end(), [](const Entry& e, const Entry& b) { return e.chance > b.chance; });

                    uint32_t sum = 0;
                    for (const auto& e : geometries)
                        sum += e.chance;

                    for (uint32_t i = 0; i < 1000; ++i)
                    {
                        uint32_t x = rnd.nextUint32() % m_width;
                        uint32_t y = rnd.nextUint32() % m_height;

                        uint32_t o = rnd.nextUint32() % sum;

                        uint32_t r = 0;
                        for (const auto& e : geometries)
                        {
                            r += e.chance;
                            if (o < r)
                            {
                                m_objects.emplaceBack(e.geom, x * TILE_SIZE + (TILE_SIZE/2), y * TILE_SIZE);
                                break;
                            }
                        }
                    }

                    std::sort(m_objects.begin(), m_objects.end(), [](const Sprite& a, const Sprite& b) { return a.pos.y < b.pos.y; });
                }
            }

        };

        //--


        /// test of basic texture atlas rendering
        class SceneTest_CanvasAtlas : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasAtlas, ICanvasTest);

        public:
            base::image::ImagePtr m_atlasImage;
            base::RefPtr<SimpleAtlas> m_atlasData;

            base::UniquePtr<GameAtlasTiles> m_atlasTiles;

            base::UniquePtr<GameLevel> m_level;

            base::NativeTimePoint m_time;

            virtual void initialize() override
            {
                m_time.resetToNow();

                m_atlasImage = loadImage("terrain_atlas.png");
                m_atlasData = base::LoadResource<SimpleAtlas>(base::res::ResourcePath("engine/tests/textures/terrain_atlas.v4atlas")).acquire();

                if (m_atlasImage && m_atlasData)
                {
                    m_atlasTiles.create(*m_atlasData, m_atlasImage);
                    m_level.create(128, 128, *m_atlasTiles);
                }
                
            }

            virtual void render(base::canvas::Canvas& c) override
            {
                if (m_level)
                {
                    // offset
                    float maxOffsetX = 640.0f;// (m_level->pixelWidth() - c.width()) * 0.5f;
                    float maxOffsetY = 640.0f;// (m_level->pixelHeight() - c.height()) * 0.5f;

                    float time = (float)m_time.timeTillNow().toSeconds();
                    float offsetX = (1.0f + cosf(time * 0.1f + 0.523f)) * maxOffsetX;
                    float offsetY = (1.0f + cosf(time * 0.5f + 0.763f)) * maxOffsetY;

                    c.pixelPlacement((int)-offsetX, (int)-offsetY);
                    c.scissorRect(offsetX, offsetY, offsetX + c.width(), offsetY + c.height());

                    m_level->render(c);
                }                
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasAtlas);
            RTTI_METADATA(CanvasTestOrderMetadata).order(120);
        RTTI_END_TYPE();

        //---

    } // test
} // rendering