/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "core/object/include/object.h"
#include "graphContainer.h"

BEGIN_BOOMER_NAMESPACE_EX(graph)

//---

/// socket description
struct CORE_GRAPH_API BlockSocketStyleInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BlockSocketStyleInfo);

    // directionality information, allows the arrows to be rendered, also prevents two inputs from connecting, etc
    SocketDirection m_direction = SocketDirection::Input;

    // socket placement, only used for rendering
    SocketPlacement m_placement = SocketPlacement::Left;

    // socket drawing style
    SocketShape m_shape = SocketShape::Block;

    // socket color
    Color m_socketColor = Color(0,0,0,255);

    // link color (on the socket side), can be overridden by the link itself
    Color m_linkColor = Color(0, 0, 0, 255);

    // socket tags, used for filtering - if socket has a given tag it can connect
    Array<StringID> m_tags;

    // excluded socket tags - if socket has a tag it cannot connect even if other tag may match
    Array<StringID> m_excludedTags;

    // output swizzle (materials graph hack)
    StringID m_swizzle;

    // multiple connections are allowed at this socket
    bool m_multiconnection = false;

    // can this socket initialize connections ?
    bool m_canInitializeConnections = true;

    // socket is visible by default
    bool m_visibleByDefault = true;

    // socket can be hidden by user
    bool m_hiddableByUser = true;

    // should the caption be displayed
    bool m_displayCaption = true;

    //---

    INLINE BlockSocketStyleInfo& direction(SocketDirection& dir) { m_direction = dir; return *this; }
    INLINE BlockSocketStyleInfo& input() { m_direction = SocketDirection::Input; return *this; }
    INLINE BlockSocketStyleInfo& output() { m_direction = SocketDirection::Output; return *this; }
    INLINE BlockSocketStyleInfo& bidirectional() { m_direction = SocketDirection::Bidirectional; return *this; }
    INLINE BlockSocketStyleInfo& placement(SocketPlacement& dir) { m_placement = dir; return *this; }
    INLINE BlockSocketStyleInfo& top() { m_placement = SocketPlacement::Top; return *this; }
    INLINE BlockSocketStyleInfo& bottom() { m_placement = SocketPlacement::Bottom; return *this; }
    INLINE BlockSocketStyleInfo& left() { m_placement = SocketPlacement::Left; return *this; }
    INLINE BlockSocketStyleInfo& right() { m_placement = SocketPlacement::Right; return *this; }
    INLINE BlockSocketStyleInfo& center() { m_placement = SocketPlacement::Center; return *this; }
    INLINE BlockSocketStyleInfo& color(Color color) { m_socketColor = color; return *this; }
    INLINE BlockSocketStyleInfo& color(uint8_t r, uint8_t g, uint8_t b) { m_socketColor = Color(r,g,b); return *this; }
    INLINE BlockSocketStyleInfo& linkColor(Color color) { m_linkColor = color; return *this; }
    INLINE BlockSocketStyleInfo& multiconnection(bool multiconnection = true) { multiconnection = multiconnection; return *this; }
    INLINE BlockSocketStyleInfo& tag(StringID txt) { if (txt) m_tags.pushBackUnique(txt); return *this; }
    INLINE BlockSocketStyleInfo& excludeTag(StringID txt) { if (txt) m_excludedTags.pushBackUnique(txt); return *this; }
    INLINE BlockSocketStyleInfo& swizzle(StringID swizzle) { m_swizzle = swizzle; return *this; }
    INLINE BlockSocketStyleInfo& hiddenByDefault() { m_visibleByDefault = false; return *this; }
    INLINE BlockSocketStyleInfo& visibleByDefault(bool flag = true) { m_visibleByDefault = flag; return *this; }
    INLINE BlockSocketStyleInfo& hideCaption() { m_displayCaption = false; return *this; }
    INLINE BlockSocketStyleInfo& displayCaption(bool flag = true) { m_displayCaption = flag; return *this; }
    INLINE BlockSocketStyleInfo& shapeInput() { m_shape = SocketShape::Input; return *this; }
    INLINE BlockSocketStyleInfo& shapeOutput() { m_shape = SocketShape::Output; return *this; }
    INLINE BlockSocketStyleInfo& shapeBidirectional() { m_shape = SocketShape::Bidirectional; return *this; }
    INLINE BlockSocketStyleInfo& shapeBlock() { m_shape = SocketShape::Block; return *this; }

    //---

    /// can we connect two sockets that have given setups
    /// NOTE: this is directional check
    static bool CanConnect(const BlockSocketStyleInfo& from, const BlockSocketStyleInfo& to);

    //---

private:
    static bool CheckDirections(const BlockSocketStyleInfo& a, const BlockSocketStyleInfo& b);
    static bool CheckTags(const BlockSocketStyleInfo& a, const BlockSocketStyleInfo& b);
};

//---

/// helper class used to define sockets of the graph block
struct CORE_GRAPH_API BlockLayoutBuilder
{
    void socket(StringID name, const BlockSocketStyleInfo& info);

    //--

    struct SocketInfo
    {
        StringID name;
        BlockSocketStyleInfo info;
    };

    typedef Array<SocketInfo> TSockets;
    TSockets collectedSockets;
};

//--

        
//---

// base graph block element
class CORE_GRAPH_API Block : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(Block, IObject);

public:
    Block();
    virtual ~Block();

    /// get the graph this block belongs to
    /// NOTE: this is valid only if block is part of the graph
    INLINE Container* graph() const { return rtti_cast<Container>(parent()); }

    // current sockets in this block
    typedef Array<SocketPtr> TSockets;
    INLINE const TSockets& sockets() const { return m_sockets; }

    // current position
    INLINE const Point& position() const { return m_position; }

    // set block position
    INLINE void position(const Point& pos) { m_position = pos; }

    // set block position
    INLINE void position(int x, int y) { m_position = Point(x,y); }

    ///--

    /// called when a layout reevaluation is requested, default pushes the block metadata
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const;

    /// called when block connections changed, allows internal cache to be updated
    virtual void handleConnectionsChanged();

    /// called when existing sockets are hidden/shown
    virtual void handleSocketLayoutChanged();

    /// rebuild block layout, posts OnLayoutChanged
    void rebuildLayout(bool postEvent=true);

    ///---

    /// find socket by name, sockets are bound to have unique names
    Socket* findSocket(StringID socketName) const;

    ///---

    /// do we have any connections on this block ?
    bool hasConnections() const;

    /// do we have any connections to specific block ?
    bool hasAnyConnectionTo(const Block* block) const;

    /// do we have any connections to specific socket ?
    bool hasAnyConnectionTo(const Socket* socket) const;

    /// do we have any connections on socket of given name (SLOW)
    bool hasConnectionOnSocket(StringID name) const;

    /// break all connections on this block
    void breakAllConnections();

    /// get all connections on this block
    void getConnections(Array<Connection*>& outConnections) const;

    /// enumerate all connections
    bool enumConnections(const std::function<bool(Connection*)>& enumFunc) const;
            
    ///---

    /// invalidate rendering style - title, colors, shape, etc
    /// NOTE: this will force the graph editor to recompute the layout
    void invalidateStyle();

    /// get shape to render the block with
    virtual BlockShape chooseBlockShape() const;

    /// get title string - can contains icons, or other styling
    virtual StringBuf chooseTitle() const;

    /// get color of the title bar - different classes of blocks have different title color
    /// NOTE: for blocks without titles this is used for the block's body
    virtual Color chooseTitleColor() const;

    /// get color of the border (for blocks with borders - circles mostly)
    virtual Color chooseBorderColor() const;

    ///--
            
private:
    TSockets m_sockets;
    Point m_position;
};

//---

// meta data with the block shape
class CORE_GRAPH_API BlockShapeMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockShapeMetadata, IMetadata);

public:
    INLINE void shape(BlockShape shape) { value = shape; }
    INLINE void rectangleWithTitle() { value = BlockShape::RectangleWithTitle; }
    INLINE void slantedWithTitle() { value = BlockShape::SlandedWithTitle; }
    INLINE void roundedWithTitle() { value = BlockShape::RoundedWithTitle; }
    INLINE void rectangle() { value = BlockShape::Rectangle; }
    INLINE void slanted() { value = BlockShape::Slanded; }
    INLINE void rounded() { value = BlockShape::Rounded; }
    INLINE void circle() { value = BlockShape::Circle; }
    INLINE void octagon() { value = BlockShape::Octagon; }
    INLINE void leftArrow() { value = BlockShape::ArrowLeft; }
    INLINE void rightArrow() { value = BlockShape::ArrowRight; }
    INLINE void leftTriangle() { value = BlockShape::TriangleLeft; }
    INLINE void rightTriangle() { value = BlockShape::TriangleRight; }

    BlockShape value = BlockShape::Rectangle;
};

//---

// meta data with the block name and group to be displayed in the block list
class CORE_GRAPH_API BlockInfoMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockInfoMetadata, IMetadata);

public:
    INLINE BlockInfoMetadata& title(const char* caption) { titleString = caption; return *this; }
    INLINE BlockInfoMetadata& group(const char* group) { groupString = group; return *this; }
    INLINE BlockInfoMetadata& name(const char* name) { nameString = name; return *this; }

    StringView titleString;
    StringView groupString;
    StringView nameString;
};
        
//---

// meta data with the block title color
class CORE_GRAPH_API BlockTitleColorMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockTitleColorMetadata, IMetadata);

public:
    INLINE void color(Color color) { value = color; }
    INLINE void color(uint8_t r, uint8_t g, uint8_t b) { value = Color(r, g, b); }

    Color value;
};

//---

// meta data with the block title color
class CORE_GRAPH_API BlockStyleNameMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockStyleNameMetadata, IMetadata);

public:
    INLINE void style(StringView txt) { name = txt; }

    StringView name;
};

//---

// meta data with the block border color
class CORE_GRAPH_API BlockBorderColorMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockBorderColorMetadata, IMetadata);

public:
    INLINE void color(Color color) { value = color; }
    INLINE void color(uint8_t r, uint8_t g, uint8_t b) { value = Color(r, g, b); }

    Color value;
};

//---

// meta data with additional description for block (if not loaded from help files)
class CORE_GRAPH_API BlockHelpMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockHelpMetadata, IMetadata);

public:
    INLINE BlockHelpMetadata& help(const char* text) { helpString = text; return *this; }

    StringView helpString;
};

//--

END_BOOMER_NAMESPACE_EX(graph)
