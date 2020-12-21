/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "gameScene.h"
#include "game.h"

namespace example
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GameSpriteAsset);
        //RTTI_PROPERTY(m_image);
    RTTI_END_TYPE();

    GameSpriteAsset::GameSpriteAsset(base::canvas::DynamicAtlas* atlas, const base::image::ImageRef& image, float scale, bool circle)
        : m_size(1,1)
        , m_scale(scale)
        , m_circle(circle)
        , m_name("Sprite")
    {
        m_image = atlas->registerImage(image.acquire());
        buildGeometry();
    }

    void GameSpriteAsset::setupSnapBottom()
    {
        if (!snapBottom)
        {
            snapBottom = true;
            buildGeometry();
        }
    }

    void GameSpriteAsset::onPropertyChanged(StringView path)
    {
        if (path == "image")
            buildGeometry();

        TBaseClass::onPropertyChanged(path);
    }

    void GameSpriteAsset::bounds(Vector2& outMin, Vector2& outMax) const
    {
        const auto extents = m_size / 2.0f;

        if (snapBottom)
        {
            outMin.x = -extents.x;
            outMax.x = extents.x;
            outMin.y = -m_size.y;
            outMax.y = 0.0f;
        }
        else
        {
            outMin = -extents;
            outMax = extents;
        }
    }

    void GameSpriteAsset::buildGeometry(GeometryBuilder& builder, bool flip) const
    {
        auto fillSettings = base::canvas::ImagePatternSettings().scale(m_scale);

        if (snapBottom)
            fillSettings.offset(-m_size.x / 2.0f, -m_size.y);
        else
            fillSettings.offset(-m_size.x / 2.0f, -m_size.y / 2.0f);

        if (flip)
        {
            fillSettings.m_scaleX = -fillSettings.m_scaleX;
            fillSettings.m_wrapU = true;
        }

        auto fillStyle = ImagePattern(m_image, fillSettings);
        builder.fillPaint(fillStyle);

        builder.beginPath();

        if (m_circle)
        {
            builder.circle(0, 0, m_size.x * 0.5f);
        }
        else
        {
            Vector2 tl, br;
            bounds(tl, br);
            builder.rect(tl, br);
        }

        builder.fill();
    }

    void GameSpriteAsset::buildGeometry()
    {
        m_size.x = m_image.width * m_scale;
        m_size.y = m_image.height * m_scale;

        {
            GeometryBuilder builder(m_geometry[0]);
            buildGeometry(builder, false);
        }

        {
			GeometryBuilder builder(m_geometry[1]);
			buildGeometry(builder, true);
        }
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GameSpriteSequenceAsset);
    RTTI_END_TYPE();

    GameSpriteSequenceAsset::GameSpriteSequenceAsset(base::canvas::DynamicAtlas* atlas, StringView baseDepotPath, uint32_t numFrames, float fps, int downsample /*= 0*/)
        : m_fps(fps)
    {
        for (uint32_t i = 1; i < numFrames; ++i)
        {
            auto image = LoadResource<Image>(TempString("{} ({}).png", baseDepotPath, i + 1)).acquire();

            for (int j=0; j<downsample; ++j)
                image = Downsampled(image->view(), DownsampleMode::AverageWithAlphaWeight, ColorSpace::SRGB);

            auto frameAsset = RefNew<GameSpriteAsset>(atlas, image);
            frameAsset->setupSnapBottom(); // HACK

            m_frameAssets.pushBack(frameAsset);
            m_size = m_frameAssets.back()->size();
        }
    }

    const Geometry& GameSpriteSequenceAsset::geometry(float time, bool flip) const
    {
        auto frame = (int)std::floorf(m_fps * time);
        return m_frameAssets[frame % m_frameAssets.size()]->geometry(flip);
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(GameObject);
    RTTI_END_TYPE();

    GameObject::GameObject()
    {}

    void GameObject::added(GameSceneLayer* layer)
    {}

    void GameObject::removed(GameSceneLayer* layer)
    {}

    void GameObject::tick(float dt)
    {}

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GameSprite);
    RTTI_END_TYPE();

    GameSprite::GameSprite(Vector2 pos, GameSpriteAsset* asset)
        : m_asset(AddRef(asset))
    {
        this->pos = pos;
    }

    void GameSprite::bounds(Vector2& outMin, Vector2& outMax) const
    {
        if (m_asset)
        {
            m_asset->bounds(outMin, outMax);
            outMin += pos;
            outMax += pos;
        }
    }

    void GameSprite::render(Canvas& canvas)
    {
        if (m_asset)
        {
            const auto placement = XForm2D::BuildRotation(rot).translation(pos);
            canvas.place(placement, m_asset->geometry());
        }

        if (m_renderBBox)
        {
			Geometry g;
			{
				GeometryBuilder builder(g);
				builder.strokeColor(Color::YELLOW);

				Vector2 tl, br;
				bounds(tl, br);
				builder.beginPath();
				builder.rect(tl, br);
				builder.stroke();
			}

            canvas.place(0, 0, g);
        }
    }

    void GameSprite::debug()
    {
        if (!m_asset)
            return;

        if (ImGui::TreeNode(this, TempString("Sprite ({})", m_asset->name())))
        {
            ImGui::Text(TempString("Position: {},{}", pos.x, pos.y));
            ImGui::Text(TempString("Size: {},{}", m_asset->size().x, m_asset->size().y));
            ImGui::Checkbox("Draw bounds", &m_renderBBox);
            ImGui::TreePop();
        }
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GameTerrianAsset)
        //RTTI_PROPERTY(m_tileImages);
    RTTI_END_TYPE();

    GameTerrianAsset::GameTerrianAsset(base::canvas::DynamicAtlas* atlas, float tileSize, StringView baseDepotPath, uint32_t numTiles)
        : m_tileSize(tileSize)
    {
        m_tileImages.resize(numTiles);

        for (uint32_t i = 1; i < numTiles; ++i)
        {
            if (auto image = LoadResource<Image>(TempString("{}/{}.png", baseDepotPath, i)))
                m_tileImages[i] = atlas->registerImage(image.acquire(), false, 0);
        }
    }

    void GameTerrianAsset::placeTile(GeometryBuilder& builder, int x, int y, char c)
    {
        if (c && c <= m_tileImages.lastValidIndex())
        {
            if (const auto image = m_tileImages[c])
            {
                const auto scale = m_tileSize / image.width;

                const auto fillSettings = base::canvas::ImagePatternSettings().scale(scale).wrapU(false).wrapV(false);
                const auto fillStyle = ImagePattern(image, fillSettings);
                builder.fillPaint(fillStyle);

                builder.pushTransform();
                builder.translate(x * m_tileSize, y * m_tileSize);

                builder.beginPath();
                builder.rect(0, 0, m_tileSize, m_tileSize);
                builder.fill();

                builder.popTransform();
            }
        }
    }

    //--
    
    RTTI_BEGIN_TYPE_NATIVE_CLASS(GameTerrain);
    RTTI_END_TYPE();

    GameTerrain::GameTerrain(uint32_t width, uint32_t height, GameTerrianAsset* asset)
        : m_width(width)
        , m_height(height)
        , m_asset(AddRef(asset))
    {
        m_tiles.resizeWith(m_width * m_height, 0);
        buildGeometry();
    }

    float GameTerrain::groundHeight(float x, float y) const
    {
        const auto tileSize = m_asset->tileSize();

        int baseX = (int)std::floor(x / tileSize);
        if (baseX >= 0 && baseX < (int)m_width)
        {
            int baseY = (int)std::floor(y / tileSize) + 1;
            while (baseY < (int)m_height)
            {
                if (tile(baseX, baseY) != 0)
                    return baseY * tileSize;
                baseY += 1;
            }
        }

        return m_height * tileSize;
    }

    char GameTerrain::tile(int x, int y) const
    {
        if (x >= 0 && y >= 0 && x < (int)m_width && y < (int)m_height)
            return m_tiles[x + y * m_width];
        return 0;
    }

    void GameTerrain::tile(int x, int y, char c)
    {
        if (x >= 0 && y >= 0 && x < (int)m_width && y < (int)m_height)
        {
            auto& current = m_tiles[x + y * m_width];
            if (current != c)
            {
                current = c;
                invalidateGeometry();
            }
        }
    }

    TileCode GameTerrain::tileCode(int x, int y) const
    {
        TileCode ret;
        ret.tl = tile(x-1, y-1) != 0;
        ret.t = tile(x, y - 1) != 0;
        ret.tr = tile(x+1, y - 1) != 0;
        ret.l = tile(x - 1, y) != 0;
        ret.c = tile(x, y) != 0;
        ret.r = tile(x + 1, y) != 0;
        ret.bl = tile(x - 1, y + 1) != 0;
        ret.b = tile(x, y + 1) != 0;
        ret.br = tile(x + 1, y + 1) != 0;
        return ret;
    }

    Vector2 GameTerrain::tileCenter(int x, int y) const
    {
        const auto size = m_asset->tileSize();
        const auto tileX = size * (x + 0.5f);
        const auto tileY = size * (y + 0.5f);
        return Vector2(tileX, tileY);
    }

    Vector2 GameTerrain::tileSnap(int x, int y) const
    {
        const auto size = m_asset->tileSize();
        const auto tileX = size * (x + 0.5f);
        const auto tileY = size * (y + 1.0f);
        return Vector2(tileX, tileY);
    }

    static char TileImageForCode(TileCode code)
    {
        if (!code.c)
            return 0;

        if (!code.t && code.l && code.r)
            return code.b ? 2 : 14;
        if (!code.t && !code.l && code.r)
            return code.b ? 1 : 13;
        if (!code.t && code.l && !code.r)
            return code.b ? 3 : 15;

        if (code.t && code.b && code.r && !code.l)
            return 4;
        else if (code.t && code.b && !code.r && code.l)
            return 6;
        else if (code.t && code.b && code.r && code.l)
            return 5;

        if (!code.b && code.l && code.r)
            return code.t ? 9 : 14;
        else if (!code.b && !code.l && code.r)
            return code.t ? 12 : 13;
        else if (!code.b && code.l && !code.r)
            return code.t ? 16 : 15;

        return 5;
    }

    void GameTerrain::render(Canvas& canvas)
    {
        if (!m_geometry)
            buildGeometry();

        canvas.place(0, 0, m_geometry);
    }

    void GameTerrain::invalidateGeometry()
    {
        m_geometry.reset();
    }

    void GameTerrain::added(GameSceneLayer* layer)
    {
        DEBUG_CHECK(m_layer == nullptr);
        m_layer = layer;
    }

    void GameTerrain::removed(GameSceneLayer* layer)
    {
        DEBUG_CHECK(m_layer == layer);
        m_layer = nullptr;
    }

    void GameTerrain::buildGeometry()
    {
        GeometryBuilder builder(m_geometry);

        if (m_asset)
        {
            for (uint32_t y = 0; y < m_height; ++y)
            {
                for (uint32_t x = 0; x < m_width; ++x)
                {
                    const auto code = tileCode(x, y);
                    const auto imageIndex = TileImageForCode(code);
                    if (imageIndex)
                        m_asset->placeTile(builder, x, y, imageIndex);
                }
            }
        }
    }

    void GameTerrain::debug()
    {
        if (ImGui::TreeNode(this, TempString("Terrain ({}x{})", m_width, m_height)))
        {
            for (uint32_t y = 0; y < m_height; ++y)
            {
                for (uint32_t x = 0; x < m_width; ++x)
                {
                    ImGui::PushID(1 + (x + y * m_width));

                    if (x > 0)
                        ImGui::SameLine();

                    char code = tile(x, y);
                    if (ImGui::SmallButton(TempString("{}", code)))
                    {
                        tile(x, y, 1 - code);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }
    }

    //---

    GameSceneLayer::GameSceneLayer()
    {}

    GameSceneLayer::~GameSceneLayer()
    {}

    void GameSceneLayer::tick(float dt)
    {
        for (auto& obj : m_objects)
            obj->tick(dt);
    }

    void GameSceneLayer::render(Canvas& canvas) const
    {
        for (auto& obj : m_objects)
            obj->render(canvas);
    }

    void GameSceneLayer::render(CommandWriter& cmd, Vector2 center, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight) const
    {
        float screenCenterX = width / 2.0f;
        float screenCenterY = height / 2.0f;

        float canvasOffsetX = center.x - screenCenterX;
        float canvasOffsetY = 0;// center.y - screenCenterY;

        Canvas::Setup setup;
        setup.width = width;
        setup.height = height;
        setup.pixelOffset.x = -canvasOffsetX;
        setup.pixelOffset.y = -canvasOffsetY;
        setup.pixelScale = 1.0f;

        Canvas canvas(setup);
        canvas.scissorBounds(canvasOffsetX, 0.0f, canvasOffsetX + width, height);
        render(canvas);

        GetService<rendering::canvas::CanvasRenderService>()->render(cmd, canvas);
    }

    void GameSceneLayer::debug(int index)
    {
        if (ImGui::TreeNode(this, TempString("Layer{} ({} objects)", index, m_objects.size())))
        {
            for (const auto& obj : m_objects)
                obj->debug();
            ImGui::TreePop();
        }
    }

    void GameSceneLayer::addObject(GameObject* object)
    {
        m_objects.pushBack(AddRef(object));
    }

    void GameSceneLayer::removeObject(GameObject* object)
    {
        m_objects.remove(object);
    }

    //---

    GameScene::GameScene()
    {}

    GameScene::~GameScene()
    {}

    void GameScene::bounds(uint32_t width, uint32_t height, Vector2& outMin, Vector2& outMax) const
    {
        float extentsX = width / 2.0f;
        float extentsY = height / 2.0f;

        outMin.x = m_centerPos.x - extentsX;
        outMin.y = m_centerPos.y - extentsY;
        outMax.x = m_centerPos.x + extentsX;
        outMax.y = m_centerPos.y + extentsY;
    }

    void GameScene::tick(float dt)
    {
        for (uint32_t i = 0; i < MAX_LAYERS; ++i)
            m_layer[i].tick(dt);
    }

    void GameScene::render(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight) const
    {
        // TODO: it's possible to do "post processes" here with the images

        for (int i = MAX_LAYERS-1; i >= 0; --i)
        {
            const float zoomScale = 1.0f - (i * 0.2f); // "perspective"
            m_layer[i].render(cmd, m_centerPos * zoomScale, width, height, outputWidth, outputHeight);
        }
    }

    //--

} // example
