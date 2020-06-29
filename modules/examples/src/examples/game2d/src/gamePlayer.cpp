/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "gamePlayer.h"

namespace example
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GamePlayerAssets);
    RTTI_END_TYPE();

    GamePlayerAssets::GamePlayerAssets(StringView<char> baseDepotFolder)
    {
        idle = CreateSharedPtr<GameSpriteSequenceAsset>(TempString("{}/Idle", baseDepotFolder), 10, 15.0f, 2);
        walk = CreateSharedPtr<GameSpriteSequenceAsset>(TempString("{}/Walk", baseDepotFolder), 10, 15.0f, 2);
        run = CreateSharedPtr<GameSpriteSequenceAsset>(TempString("{}/Run", baseDepotFolder), 8, 15.0f, 2);
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GamePlayer);
    RTTI_END_TYPE();

    GamePlayer::GamePlayer(Vector2 pos, GamePlayerAssets* assets)
        : m_assets(AddRef(assets))
        , m_spawnPos(pos)
    {
        m_onGround = false;
        this->pos = pos;
    }

    void GamePlayer::bounds(Vector2& outMin, Vector2& outMax) const
    {
        const auto size = m_assets->idle->size();

        outMin.x = pos.x - (size.x / 2.0f);
        outMin.y = pos.y - size.y;
        outMax.x = pos.x + (size.x / 2.0f);
        outMax.y = pos.y;
    }

    bool GamePlayer::handleInput(const BaseEvent& evt)
    {
        if (const auto* key = evt.toKeyEvent())
        {
            if (key->keyCode() == KeyCode::KEY_A)
                m_buttonLeft = key->isDown();
            else if (key->keyCode() == KeyCode::KEY_D)
                m_buttonRight = key->isDown();
            else if (key->keyCode() == KeyCode::KEY_W)
                m_buttonUp = key->isDown();
            else if (key->keyCode() == KeyCode::KEY_S)
                m_buttonDown = key->isDown();
            else if (key->keyCode() == KeyCode::KEY_SPACE)
                m_buttonJump = key->isDown();
            else if (key->keyCode() == KeyCode::KEY_LEFT_SHIFT)
                m_buttonRun = key->isDown();
            
            return true;
        }

        return false;
    }

    float ReachWithAcc(float val, float target, float acc, float dt)
    {
        auto maxChange = acc * dt;
        if (target > val)
        {
            auto delta = std::min<float>(target - val, maxChange);
            val += delta;
        }
        else if (target < val)
        {
            auto delta = std::min<float>(val - target, maxChange);
            val -= delta;
        }

        return val;
    }

    void GamePlayer::tick(float dt)
    {
        Vector2 delta(0, 0);

        if (m_buttonLeft)
            delta.x -= 1.0f;
        if (m_buttonRight)
            delta.x += 1.0f;
        if (m_buttonUp)
            delta.y -= 1.0f;
        if (m_buttonDown)
            delta.y += 1.0f;

        const auto speed = m_running ? 1000.0f : 500.0f;
        const auto acc = m_onGround ? 3000.0f : 1200.0f;
        const auto requestedVelX = speed * delta.x;
        m_velocity.x = ReachWithAcc(m_velocity.x, requestedVelX, acc, dt);
        m_velocity.y += 1000.0f * dt;

        float groundHeight = m_collisionTerrain->groundHeight(pos.x, pos.y - 2.0f);

        pos += m_velocity * dt;

        if (m_velocity.x < 0.0f && !m_flip)
        {
            m_flip = true;
            pos.x -= 30;
        }
        else if (m_velocity.x > 0.0f && m_flip)
        {
            m_flip = false;
            pos.x += 30;
        }

        if (pos.y >= groundHeight)
        {
            pos.y = groundHeight;
            m_velocity.y = 0.0f;
            m_onGround = true;
            m_running = m_buttonRun;
        }
        else
        {
            m_onGround = false;

            if (!m_buttonRun)
                m_running = false;
        }

        if (m_onGround && m_buttonJump)
            m_velocity.y = -700.0f;

        int state = 0;
        if (fabs(m_velocity.x) > 600.0f)
            state = 2;
        else if (fabs(m_velocity.x) > 50.0f)
            state = 1;

        changeState(state, dt);

        if (pos.y > 1400.0f)
        {
            pos = m_spawnPos;
            m_velocity = Vector2(0, 0);
        }
    }
    
    void GamePlayer::changeState(int state, float dt)
    {
        if (state != m_state)
        {
            m_state = state;
            m_timeInState = 0.0f;
        }
        else
        {
            if (state != 0)
                m_timeInState += dt * (m_onGround ? 1.0f : 0.33f);
            else
                m_timeInState += dt;
        }
    }

    void GamePlayer::render(Canvas& canvas)
    {
        const auto placement = XForm2D::BuildRotation(rot).translation(pos + Vector2(0,13)); // HACK
        canvas.placement(placement);

        if (m_state == 0)
            canvas.place(m_assets->idle->geometry(m_timeInState, m_flip));
        else if (m_state == 1)
            canvas.place(m_assets->walk->geometry(m_timeInState, m_flip));
        else if (m_state == 2)
            canvas.place(m_assets->run->geometry(m_timeInState, m_flip));
    }

    //--

} // example
