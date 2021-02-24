/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(game)

//--

/// helper class doing the top-level world loading/unloading and switching
class GameWorldTransition : public base::IReferencable
{
public:
    WorldPtr world;

    bool showLoadingScreen = true;
    GameWorldStackOperation op = GameWorldStackOperation::Activate;

    base::UniquePtr<GameTransitionInfo> transitionData;
    base::UniquePtr<GameStartInfo> initData;

    std::atomic<bool> flagCanceled = false;
    std::atomic<bool> flagFinished = false;

    //--

    void process();

private:
    bool processTransition(const GameTransitionInfo& setup);
    bool processInit(const GameStartInfo& setup);
};

//--

END_BOOMER_NAMESPACE(game)
