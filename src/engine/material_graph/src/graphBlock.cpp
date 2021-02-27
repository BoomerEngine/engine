/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlock);
    RTTI_METADATA(graph::BlockShapeMetadata).rectangleWithTitle();
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

MaterialGraphBlock::MaterialGraphBlock()
{}

MaterialGraphBlock::~MaterialGraphBlock()
{}

///---

/*static inline const Color COLOR_MATH = Color::MEDIUMORCHID;
static inline const Color COLOR_ATTRIBUTE = Color::CADETBLUE;
static inline const Color COLOR_CONST = "#51574A"; Color::DIMGRAY;
static inline const Color COLOR_GLOBAL = Color::SLATEBLUE;
static inline const Color COLOR_TEXTURE = Color::FIREBRICK;
static inline const Color COLOR_STREAM = Color::DARKSEAGREEN;
static inline const Color COLOR_OUTPUT = Color::CORAL;
static inline const Color COLOR_VECTOR = Color::CORAL;
static inline const Color COLOR_GENERIC = Color(255, 128, 64); // classic*/

const Color::ColorVal COLOR_SOCKET_RED = Color(200, 70, 70).toRGBA();
const Color::ColorVal COLOR_SOCKET_GREEN = Color(70, 200, 70).toRGBA();
const Color::ColorVal COLOR_SOCKET_BLUE = Color(70, 70, 200).toRGBA();
const Color::ColorVal COLOR_SOCKET_ALPHA = Color(130, 130, 130).toRGBA();
const Color::ColorVal COLOR_SOCKET_TEXTURE = Color(150, 100, 180).toRGBA();
const Color::ColorVal COLOR_SOCKET_CUBE = Color(225, 100, 80).toRGBA();

///---

END_BOOMER_NAMESPACE()
