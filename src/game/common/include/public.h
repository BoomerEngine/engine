/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "game_common_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

class GameScreenStack;
typedef RefPtr<GameScreenStack> GameScreenStackPtr;
    
struct GameInitParams;
struct GamePlayerInitParams;

class IGame;
typedef RefPtr<IGame> GamePtr;

class IGameScreen;
typedef RefPtr<IGameScreen> GameScreenPtr;
typedef SpecificClassType<IGameScreen> GameScreenClass;

class IGameLoadingScreen;
typedef RefPtr<IGameLoadingScreen> GameLoadingScreenPtr;

class IGamePlayer;
typedef RefPtr<IGamePlayer> GamePlayerPtr;

class IGamePlayerLocal;
typedef RefPtr<IGamePlayerLocal> GamePlayerLocalPtr;

class IGamePlayerEntity;
typedef RefPtr<IGamePlayerEntity> GamePlayerEntityPtr;

class IGamePlayerIdentity;
typedef RefPtr<IGamePlayerIdentity> GamePlayerIdentityPtr;

class IGamePlatform;
typedef RefPtr<IGamePlatform> GamePlatformPtr;

class IGamePlatformEvent;
typedef RefPtr<IGamePlatformEvent> GamePlatformEventPtr;

class IGamePlayerConnection;
typedef RefPtr<IGamePlayerConnection> GamePlayerConnectionPtr;

class IGameScreenSinglePlayer;

class GamePlatformService;

//--

struct GameInputAction;

class GameInputActionTable;
typedef RefPtr<GameInputActionTable> InputActionTablePtr;

class GameInputContext;
typedef RefPtr<GameInputContext> InputContextPtr;

class GameInputEvent;
typedef RefPtr<GameInputEvent> GameInputEventPtr;

class GameInputDefinitions;
typedef RefPtr<GameInputDefinitions> InputDefinitionsPtr;
typedef ResourceRef<GameInputDefinitions> InputDefinitionsRef;

//--

END_BOOMER_NAMESPACE()
