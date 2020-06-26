/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#pragma once

namespace base
{
    namespace stream
    {

        enum class BinaryStreamFlags
        {
            Error = FLAG(1),
            MemoryBased = FLAG(2),
            Reader = FLAG(3),
            Writer = FLAG(4),
            NullFile = FLAG(5),
        };

    } // stream

} // base