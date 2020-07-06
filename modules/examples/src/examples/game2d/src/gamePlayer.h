/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

#include "gameScene.h"

namespace example
{
    //--

    // player assets
    class GamePlayerAssets : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GamePlayerAssets, base::IObject);

    public:
        GamePlayerAssets(StringView<char> baseDepotFolder);

        GameSpriteSequenceAssetPtr idle;
        GameSpriteSequenceAssetPtr walk;
        GameSpriteSequenceAssetPtr run;
        GameSpriteSequenceAssetPtr jump;
    };

    typedef RefPtr<GamePlayerAssets> GamePlayerAssetsPtr;

    //--

    // game player
    class GamePlayer : public GameObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GamePlayer, GameObject);

    public:
        GamePlayer(Vector2 pos, GamePlayerAssets* assets);

        bool handleInput(const BaseEvent& evt);
        void bounds(Vector2& outMin, Vector2& outMax) const;

        virtual void tick(float dt) override;
        virtual void render(Canvas& canvas) override;
        virtual void debug() override;

        GameTerrain* m_collisionTerrain = nullptr; // hack

    private:
        bool m_buttonLeft = false;
        bool m_buttonRight = false;
        bool m_buttonUp = false;
        bool m_buttonDown = false;
        bool m_buttonRun = false;
        bool m_buttonJump = false;

        bool m_flip = false;
        bool m_running = false;
        bool m_onGround = false;

        float m_timeInState = 0.0f;
        int m_state = 0;

        void changeState(int state, float dt);

        Vector2 m_spawnPos;
        Vector2 m_velocity = Vector2(0, 0);

        GamePlayerAssetsPtr m_assets;
    };

    typedef RefPtr<GamePlayer> GamePlayerPtr;

    //--

} // example

