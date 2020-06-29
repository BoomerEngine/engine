/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

#include "gameScene.h"
#include "gamePlayer.h"

namespace example
{

    ///--

    // 2D game class
    class Game : public IReferencable
    {
    public:
        Game();

        void tick(float dt);
        void debug();
        void render(CommandWriter& cmd, uint32_t width, uint32_t height, const rendering::ImageView& colorTarget, const rendering::ImageView& depthTarget);
        bool handleInput(const BaseEvent& evt);
        
    private:
        void renderBackground(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight);
        void renderLevel(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight);

        void createLevel();
        void createPlayer(Vector2 pos);

        void scrollView(uint32_t width, uint32_t height);

        float m_totalTime = 0.0f;

        GameTerrainPtr m_terrain;
        GameScenePtr m_scene;
        GamePlayerPtr m_player;
    };

    ///--

} // example


