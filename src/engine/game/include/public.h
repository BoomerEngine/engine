/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_game_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

class Host;
    
class IViewportRenderer;

struct HostViewport;

class Game;
typedef RefPtr<Game> GamePtr;

class GameWorld;
typedef RefPtr<GameWorld> GameWorldPtr;

class GameGameWorldTransition;

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

class FreeCameraHelper;

//--

END_BOOMER_NAMESPACE()
