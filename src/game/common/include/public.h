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

struct InputAction;

class InputActionTable;
typedef RefPtr<InputActionTable> InputActionTablePtr;

class InputContext;
typedef RefPtr<InputContext> InputContextPtr;

class InputEvent;
typedef RefPtr<InputEvent> InputEventPtr;

class InputDefinitions;
typedef RefPtr<InputDefinitions> InputDefinitionsPtr;
typedef ResourceRef<InputDefinitions> InputDefinitionsRef;

//--

END_BOOMER_NAMESPACE()
