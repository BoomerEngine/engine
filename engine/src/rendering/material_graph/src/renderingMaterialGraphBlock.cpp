/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlock);
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangleWithTitle();
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGeneric");
    RTTI_END_TYPE();

    MaterialGraphBlock::MaterialGraphBlock()
    {}

    MaterialGraphBlock::~MaterialGraphBlock()
    {}

    ///---

    /*static inline const base::Color COLOR_MATH = base::Color::MEDIUMORCHID;
    static inline const base::Color COLOR_ATTRIBUTE = base::Color::CADETBLUE;
    static inline const base::Color COLOR_CONST = "#51574A"; base::Color::DIMGRAY;
    static inline const base::Color COLOR_GLOBAL = base::Color::SLATEBLUE;
    static inline const base::Color COLOR_TEXTURE = base::Color::FIREBRICK;
    static inline const base::Color COLOR_STREAM = base::Color::DARKSEAGREEN;
    static inline const base::Color COLOR_OUTPUT = base::Color::CORAL;
    static inline const base::Color COLOR_VECTOR = base::Color::CORAL;
    static inline const base::Color COLOR_GENERIC = base::Color(255, 128, 64); // classic*/

    const base::Color::ColorVal COLOR_SOCKET_RED = base::Color(200, 70, 70).toABGR();
    const base::Color::ColorVal COLOR_SOCKET_GREEN = base::Color(70, 200, 70).toABGR();;
    const base::Color::ColorVal COLOR_SOCKET_BLUE = base::Color(70, 70, 200).toABGR();;
    const base::Color::ColorVal COLOR_SOCKET_ALPHA = base::Color(130, 130, 130).toABGR();;
    const base::Color::ColorVal COLOR_SOCKET_TEXTURE = base::Color(150, 100, 180).toABGR();;
    const base::Color::ColorVal COLOR_SOCKET_CUBE = base::Color(225, 100, 80).toABGR();;

    ///---

} // rendering
