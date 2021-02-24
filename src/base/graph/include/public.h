/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_graph_glue.inl"

BEGIN_BOOMER_NAMESPACE(base::graph)

enum class BlockShape
{
    // shapes with title bars
    // NOTE: allows for "title" sockets but not the "top" socket
    RectangleWithTitle,
    SlandedWithTitle,
    RoundedWithTitle,

    // pure shapes with not title bars, better for more vertical layouts
    // NOTE: no "title" socket but the top sockets are allowed (with captions)
    Rectangle,
    Slanded,
    Rounded,
    Circle, // a constant size circle, NOTE: does not adapt well to big content
    Octagon, // stop sing, similar to the circle
    TriangleRight, // a triangle with tip facing right, NOTE: only left/right sockets are allowed
    TriangleLeft, // a triangle with the facing left, NOTE: only left/right sockets are allowed
    ArrowRight, // an arrow with tip facing right, NOTE: only left/right sockets are allowed
    ArrowLeft, // an arrow with tip facing left, NOTE: only left/right sockets are allowed
            
};

enum class SocketDirection
{
    Input,
    Output,
    Bidirectional,
};

enum class SocketShape
{
    Block,
    Input,
    Output,
    Bidirectional,
};

enum class SocketPlacement
{
    Top,
    Bottom,
    Left,
    Right,
    Title,
    Center,
};

typedef uint32_t BlockID;

class Block;
typedef RefPtr<Block> BlockPtr;

class Socket;
typedef RefPtr<Socket> SocketPtr;
typedef RefWeakPtr<Socket> SocketWeakPtr;

class Connection;
typedef RefPtr<Connection> ConnectionPtr;
typedef RefWeakPtr<Connection> ConnectionWeakPtr;

class Container;
typedef RefPtr<Container> ContainerPtr;

struct BlockStyleInfo;
struct BlockSocketStyleInfo;

class IGraphObserver;

END_BOOMER_NAMESPACE(base::graph)

//--

BEGIN_BOOMER_NAMESPACE(base)

class InstanceVarBase;
class InstanceArrayBase;

class InstanceBufferLayoutBuilder;

class InstanceBuffer;
typedef RefPtr<InstanceBuffer> InstanceBufferPtr;

class InstanceBufferLayout;
typedef RefPtr<InstanceBufferLayout> InstanceBufferLayoutPtr;

// create instance buffer for given buffer layout
extern BASE_GRAPH_API InstanceBufferPtr CreateBufferInstance(const InstanceBufferLayoutPtr& bufferLayout, const PoolTag poolID);

END_BOOMER_NAMESPACE(base)

//--


