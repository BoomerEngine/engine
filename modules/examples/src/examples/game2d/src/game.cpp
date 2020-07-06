/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "game.h"
#include "gameEffects.h"
#include "gameScene.h"

namespace example
{
    //---

    Game::Game()
    {
        m_scene = CreateSharedPtr<GameScene>();

        createLevel();

        const auto spawnPoint = m_terrain->tileSnap(15, 2);
        createPlayer(spawnPoint);

        m_scene->m_centerPos.x = spawnPoint.x;
    }

    void Game::tick(float dt)
    {
        m_totalTime += dt;
        m_scene->tick(dt);
    }

    bool Game::handleInput(const BaseEvent& evt)
    {
        if (m_player)
            if (m_player->handleInput(evt))
                return true;

        return false;
    }
    
    void Game::render(CommandWriter& cmd, uint32_t width, uint32_t height, const rendering::ImageView& colorTarget, const rendering::ImageView& depthTarget)
    {
        FrameBuffer fb;
        fb.color[0].view(colorTarget).clear(0.2, 0.2f, 0.2f, 1.0f); // comment the "clear" to save on fill rate
        fb.depth.view(depthTarget).clearDepth().clearStencil();
        cmd.opBeingPass(fb);

        scrollView(width, height);

        renderBackground(cmd, width, height, colorTarget.width(), colorTarget.height());
        renderLevel(cmd, width, height, colorTarget.width(), colorTarget.height());

        cmd.opEndPass();
    }

    void Game::renderBackground(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight)
    {
        RenderSky(cmd, width, height, m_scene->m_centerPos, m_totalTime);
    }

    void Game::renderLevel(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight)
    {
        m_scene->render(cmd, width, height, outputWidth, outputHeight);
    }

    void Game::createLevel()
    {
        const auto TERRAIN_WIDTH = 100;
        const auto TERRAIN_HEIGHT = 14;

        auto terrainAssets = CreateSharedPtr<GameTerrianAsset>(128.0f, "examples/canvas/tiles", 18);
        auto terrain = CreateSharedPtr<GameTerrain>(TERRAIN_WIDTH, TERRAIN_HEIGHT, terrainAssets);
        m_terrain = terrain;
        m_scene->m_layer[0].addObject(terrain);

        srand(0);

        const auto h = TERRAIN_HEIGHT / 2;

        // create terrain
        for (uint32_t y = 0; y < h; ++y)
        {
            for (uint32_t x = 0; x < TERRAIN_WIDTH; ++x)
            {
                //terrain->tile(x, y, 1); // ground
                terrain->tile(x, y + h, 1); // ground

                if ((x < 2) || (x >= 10 && x <= 13) || (x >= 30 && x <= 36) || (x > 40))
                    terrain->tile(x, y + h, 0); // ground
            }
        }

        // Assets
        {
            base::Array<GameSpriteAssetPtr> assets;

            assets.pushBack(nullptr);
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Tree_2.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Tree_3.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Mushroom_1.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Mushroom_2.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Mushroom_2.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Sign_1.png"))));
            assets.pushBack(CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/object/Sign_2.png"))));

            for (uint32_t x = 0; x < TERRAIN_WIDTH; ++x)
            {
                auto asset = assets[rand() % assets.size()];
                if (asset)
                {
                    for (uint32_t y = 0; y < TERRAIN_HEIGHT; ++y)
                    {
                        const auto code = terrain->tileCode(x, y);

                        if (!code.c && code.b)
                        {
                            const auto big = asset->size().x > terrainAssets->tileSize();
                            if (big && (code.l || code.r || !code.bl || !code.br))
                                continue;

                            auto pos = terrain->tileSnap(x, y);
                            pos.y -= (asset->size().y * 0.5f);
                            auto sprite = CreateSharedPtr<GameSprite>(pos, asset);
                            m_scene->m_layer[0].addObject(sprite);
                        }
                    }
                }
            }
        }

        // Cloud near
        {
            const auto asset = CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/cloud/cloud2.png")));
            for (int i = 0; i < 20; ++i)
            {
                float x = (TERRAIN_WIDTH * (rand() / (float)RAND_MAX)) * terrainAssets->tileSize();
                float y = (0.0f + 4 * (rand() / (float)RAND_MAX)) * terrainAssets->tileSize();

                auto sprite = CreateSharedPtr<GameSprite>(Vector2(x,y), asset);
                m_scene->m_layer[1].addObject(sprite);
            }
        }

        // Cloud far
        /*{
            const auto asset = CreateSharedPtr<GameSpriteAsset>(LoadResource<Image>(ResourcePath("examples/canvas/cloud/cloud3.png")));
            for (int i = 0; i < 10; ++i)
            {
                float x = (TERRAIN_WIDTH * (rand() / (float)RAND_MAX)) * terrainAssets->tileSize();
                float y = (4.0f + 4 * (rand() / (float)RAND_MAX)) * terrainAssets->tileSize();

                auto sprite = CreateSharedPtr<GameSprite>(Vector2(x, y), asset);
                m_scene->m_layer[2].addObject(sprite);
            }
        }*/
    }

    void Game::createPlayer(Vector2 pos)
    {
        auto asset = CreateSharedPtr<GamePlayerAssets>("examples/canvas/dino");
        m_player = CreateSharedPtr<GamePlayer>(pos, asset);
        m_player->m_collisionTerrain = m_terrain; // HACK
        m_scene->m_layer[0].addObject(m_player);
    }

    void Game::scrollView(uint32_t width, uint32_t height)
    {
        if (m_player && m_scene)
        {
            Vector2 playerMin, playerMax;
            m_player->bounds(playerMin, playerMax);

            Vector2 sceneMin, sceneMax;
            m_scene->bounds(width, height, sceneMin, sceneMax);

            sceneMin.x += width * 0.2f;
            sceneMax.x -= width * 0.2f;

            Vector2 delta(0, 0);

            if (playerMin.x < sceneMin.x)
                delta.x = playerMin.x - sceneMin.x;
            else if (playerMax.x > sceneMax.x)
                delta.x = playerMax.x - sceneMax.x;

            if (playerMin.y < sceneMin.y)
                delta.y = playerMin.y - sceneMin.y;
            else if (playerMax.y > sceneMax.y)
                delta.y = playerMax.y - sceneMax.y;

            m_scene->m_centerPos += delta;
        }
    }
    
    //---

    ConfigProperty<bool> cvDebugPageGameEntities("DebugPage.Game.Entities", "IsVisible", false);

    void Game::debug()
    {
        if (!cvDebugPageGameEntities.get())
            return;

        if (!ImGui::Begin("Entities", &cvDebugPageGameEntities.get()))
            return;

        for (uint32_t i = 0; i < GameScene::MAX_LAYERS; ++i)
            m_scene->m_layer[i].debug(i);

        ImGui::End();
    }

    //--

} // example

