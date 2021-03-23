/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#include "build.h"
#include "uiInputAction.h"
#include "uiGraphEditor.h"
#include "uiGraphEditorNode.h"
#include "uiWindow.h"
#include "uiMenuBar.h"

#include "engine/canvas/include/canvas.h"
#include "core/image/include/image.h"
#include "core/graph/include/graphContainer.h"
#include "core/graph/include/graphSocket.h"
#include "core/graph/include/graphBlock.h"
#include "core/object/include/actionHistory.h"
#include "engine/canvas/include/geometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_CLASS(GraphEditor);
    RTTI_METADATA(ElementClassNameMetadata).name("GraphEditor");
RTTI_END_TYPE();

GraphEditor::GraphEditor()
{
    hitTest(HitTestState::Enabled);
    allowFocusFromKeyboard(true);

    bindShortcut("Ctrl+C") = [this]() { actionCopySelection(); };
    bindShortcut("Ctrl+X") = [this]() { actionCutSelection(); };
    bindShortcut("Ctrl+V") = [this]() { actionPasteSelection(false, VirtualPosition(16, 16)); };
    bindShortcut("Delete") = [this]() { actionDeleteSelection(); };
}

GraphEditor::~GraphEditor()
{
    deleteVisualizations();
}

void GraphEditor::bindGraph(const graph::ContainerPtr& graph)
{
    if (m_graph != graph)
    {
        deleteVisualizations();
        m_graph = graph;
        createVisualizations();
    }
}

void GraphEditor::bindActionHistory(const ActionHistoryPtr& actionHistory)
{
    m_actionHistory = actionHistory;
}

const graph::Socket* GraphEditor::findSocketAtAbsolutePos(const Point& pos) const
{
    const auto margin = 20.0f * cachedStyleParams().pixelScale;
    auto* newBlock = rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(pos, margin));
    return newBlock ? newBlock->socketAtAbsolutePos(pos.toVector()) : nullptr;
}

bool GraphEditor::calcSocketLinkPlacement(const graph::Socket* socket, Position& outSocketAbsolutePosition, Vector2& outSocketDirection) const
{
    if (nullptr != socket)
    {
        if (auto block = socket->block())
        {
            if (auto node = m_nodesMap.findSafe(block, nullptr))
            {
                if (node->calcSocketLinkPlacement(socket, outSocketAbsolutePosition, outSocketDirection))
                {
                    outSocketAbsolutePosition += node->actualPosition() + m_viewOffset + cachedDrawArea().absolutePosition();
                    return true;
                }
            }
        }
    }

    return false;
}

void GraphEditor::enumerateSelectedBlocks(const std::function<bool(graph::Block*)>& enumFunc) const
{

}

void GraphEditor::deleteVisualizations()
{
    if (m_graph)
        m_graph->dettachObserver(this);

    m_nodesMap.clear();
    auto oldNodes = std::move(m_nodes);
    for (const auto& node : oldNodes)
        detachChild(node);
}

void GraphEditor::createVisualizations()
{
    if (m_graph)
    {
        // create block representations
        for (const auto& block : m_graph->blocks())
            handleBlockAdded(block);

        m_graph->attachObserver(this);
    }
}

void GraphEditor::handleBlockAdded(graph::Block* block)
{
    DEBUG_CHECK_EX(!m_nodesMap.contains(block), "Block already seen");
    if (!m_nodesMap.contains(block))
    {
        auto node = RefNew<GraphEditorBlockNode>(block);
        //node->parent(this);
        node->refreshBlockStyle();

        m_nodes.pushBack(node);
        m_nodesMap[block] = node;
        attachChild(node);
    }
}

void GraphEditor::handleBlockRemoved(graph::Block* block)
{
    auto node = m_nodesMap.findSafe(block, nullptr);
    DEBUG_CHECK_EX(node, "Block never seen");
    if (node)
    {
        detachChild(node);
        m_nodes.remove(node);
        m_nodesMap.remove(block);
    }
}

void GraphEditor::handleBlockStyleChanged(graph::Block* block)
{
    if (auto node = m_nodesMap.findSafe(block, nullptr))
        node->refreshBlockStyle();

}

void GraphEditor::handleBlockLayoutChanged(graph::Block* block)
{
    if (auto node = m_nodesMap.findSafe(block, nullptr))
        node->refreshBlockSockets();
}

void GraphEditor::handleBlockConnectionsChanged(graph::Block* block)
{

}

bool GraphEditor::handleMouseMovement(const InputMouseMovementEvent& evt)
{
    refreshHoverFromPosition(evt.absolutePosition().toVector());
    return TBaseClass::handleMouseMovement(evt);
}

void GraphEditor::updateHover(GraphEditorBlockNode* block, const graph::Socket* socket)
{
    if (m_hoverBlock != block || m_hoverSocket != socket)
    {
        if (m_hoverBlock == block && m_hoverBlock)
        {
            if (auto block = m_hoverBlock.lock())
                block->updateHoverSocket(socket);
            m_hoverSocket = socket;
        }
        else
        {
            if (auto block = m_hoverBlock.lock())
                block->updateHoverSocket(nullptr);

            m_hoverBlock = block;
            m_hoverSocket = socket;

            if (auto block = m_hoverBlock.lock())
                block->updateHoverSocket(m_hoverSocket.lock());
        }
    }
}

void GraphEditor::resetHover()
{
    if (m_hoverBlock)
    {
        if (auto block = m_hoverBlock.lock())
            block->updateHoverSocket(m_hoverSocket.lock());
        m_hoverBlock = nullptr;
        m_hoverSocket = nullptr;
    }
}

void GraphEditor::refreshHoverFromPosition(const Position& pos)
{
    m_lastHoverPosition = pos;
    m_lastHoverPositionValid = true;

    const auto margin = 20.0f * cachedStyleParams().pixelScale;
    auto* newBlock = rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(pos, margin));
    auto* newSocket = newBlock ? newBlock->socketAtAbsolutePos(pos) : nullptr;

    updateHover(newBlock, newSocket);
}

void GraphEditor::refreshHoverFromLastValidPosition()
{
    if (m_lastHoverPositionValid)
        refreshHoverFromPosition(m_lastHoverPosition);
}

void GraphEditor::handleHoverLeave(const Position& absolutePosition)
{
    resetHover();
    m_lastHoverPositionValid = false;

    TBaseClass::handleHoverLeave(absolutePosition);
}

void GenerateConnectionGeometry(const Position& sourcePos, const Vector2& sourceDir, const Position& targetPos, const Vector2& targetDir, Color linkColor, float width, float pixelScale, bool startArrow, bool endArrow, canvas::GeometryBuilder& builder)
{
    // curve params
    auto dist = std::max<float>(std::fabsf(sourcePos.x - targetPos.x), std::fabsf(sourcePos.y - targetPos.y));
    auto offset = (5.0f + std::min<float>(dist / 3.0f, 200.0f)) * pixelScale; // limit curvature on short links

    // main curve
    builder.beginPath();
    builder.strokeColor(linkColor, width * pixelScale);
    builder.moveTo(sourcePos);
    builder.bezierTo(sourcePos + (sourceDir * offset), targetPos + (targetDir * offset), targetPos);
    //builder.lineTo(targetPos);
    builder.stroke();

    // start arrow
    if (startArrow)
    {
        auto dir = sourceDir;
        if (dir.x == 0 && dir.y == 0) // multi directional socket
            dir = (sourcePos - targetPos).normalized();

        const auto arrowSize = (4.0f + width) * pixelScale;

        builder.beginPath();
        builder.moveTo(sourcePos);
        builder.lineTo(sourcePos - (arrowSize * dir) + (arrowSize * 0.5f * dir.prep()));
        builder.lineTo(sourcePos - (arrowSize * dir) - (arrowSize * 0.5f * dir.prep()));
        builder.fill();
    }

    // end arrow
    if (endArrow)
    {
        auto dir = targetDir;
        if (dir.x == 0 && dir.y == 0) // multi directional socket
            dir = (targetPos - sourcePos).normalized();

        const auto arrowSize = (4.0f + width) * pixelScale;
        builder.beginPath();
        builder.moveTo(targetPos);
        builder.lineTo(targetPos - (arrowSize * dir) + (arrowSize * 0.5f * dir.prep()));
        builder.lineTo(targetPos - (arrowSize * dir) - (arrowSize * 0.5f * dir.prep()));
        builder.fill();
    }
}

namespace prv
{

    class GraphCreateConnectionInputAction : public MouseInputAction
    {
    public:
        GraphCreateConnectionInputAction(GraphEditor* editor, RefWeakPtr<graph::Socket> sourceSocket, const Point& initialPoint)
            : MouseInputAction(editor, InputKey::KEY_MOUSE0)
            , m_lastAbsolutePoint(initialPoint)
            , m_sourceSocket(sourceSocket)
            , m_editor(editor)
        {
		}

        virtual void onFinished() override
        {
            if (auto validSourceSocket = m_sourceSocket.lock())
            {
                if (auto validDestDocket = m_foundTargetSocket.lock())
                    m_editor->actionConnectSockets(validSourceSocket, validDestDocket);
                else if (!m_foundTargetSocket)
                    m_editor->tryCreateNewConnectedBlockAtPosition(validSourceSocket, m_lastAbsolutePoint);
            }
        }

        virtual void onUpdateCursor(CursorType& outCursorType) override
        {
            if (auto target = m_foundTargetSocket.lock())
            {
                if (m_foundTargetSocketValid)
                    outCursorType = CursorType::Hand;
                else
                    outCursorType = CursorType::No;
            }
            else
            {
                outCursorType = CursorType::Default;
            }
        }

        virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
        {
            m_lastAbsolutePoint = evt.absolutePosition();
            updateHoverSocket();
            return InputActionResult();
        }

        virtual InputActionResult onUpdate(float dt) override
        {
            if (m_editor->autoScrollNearEdge(m_lastAbsolutePoint, dt))
            {                    
                updateHoverSocket();
            }

            return InputActionResult();
        }

        virtual void onRender(canvas::Canvas& canvas) override
        {
            if (auto validSourceSocket = m_sourceSocket.lock())
            {
                Position sourcePos(0, 0), sourceDir(0, 0);
                if (m_editor->calcSocketLinkPlacement(validSourceSocket, sourcePos, sourceDir))
                {
                    auto validDestDocket = m_foundTargetSocket.lock();

                    Position targetPos(0, 0), targetDir(0, 0);
                    targetPos = m_lastAbsolutePoint.toVector();

                    if (validDestDocket && m_foundTargetSocketValid)
                        m_editor->calcSocketLinkPlacement(validDestDocket, targetPos, targetDir);

                    canvas.pushScissorRect();
                    if (canvas.scissorRect(m_editor->cachedDrawArea().absolutePosition(), m_editor->cachedDrawArea().size()))
                    {
						canvas::Geometry geometry;

						{
							canvas::GeometryBuilder builder(geometry);

							const auto pixelWidth = m_editor->cachedStyleParams().pixelScale;
							GenerateConnectionGeometry(sourcePos, sourceDir, targetPos, targetDir, validSourceSocket->info().m_linkColor, 4.0f, pixelWidth, false, false, builder);
						}

						canvas.place(canvas::Placement(), geometry);
                    }
                    canvas.popScissorRect();
                }
            }
        }

        virtual bool allowsHoverTracking() const override
        {
            return false;
        }

        virtual bool fullMouseCapture() const override
        {
            return false;
        }

        void updateHoverSocket()
        {
            m_foundTargetSocketValid = false;
            m_foundTargetSocket = m_editor->findSocketAtAbsolutePos(m_lastAbsolutePoint);

            if (m_foundTargetSocket && m_foundTargetSocket != m_sourceSocket)
            {
                if (auto validSourceSocket = m_sourceSocket.lock())
                {
                    if (auto validDestDocket = m_foundTargetSocket.lock())
                    {
                        m_foundTargetSocketValid = graph::Socket::CanConnect(*validSourceSocket, *validDestDocket);
                    }
                }                    
            }
        }

    private:
        GraphEditor* m_editor;
        Point m_lastAbsolutePoint;

        RefWeakPtr<graph::Socket> m_sourceSocket = nullptr;
        RefWeakPtr<graph::Socket> m_foundTargetSocket = nullptr;
        bool m_foundTargetSocketValid = false;
    };

} // prv

InputActionPtr GraphEditor::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    if (evt.leftClicked())
        if (m_hoverBlock && m_hoverSocket)
            return RefNew<prv::GraphCreateConnectionInputAction>(this, m_hoverSocket, evt.absolutePosition());

    return TBaseClass::handleMouseClick(area, evt);
}

InputActionPtr GraphEditor::handleOverlayMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    if (m_hoverBlock && m_hoverSocket)
        return InputActionPtr();

    return TBaseClass::handleOverlayMouseClick(area, evt);
}

bool GraphEditor::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, CursorType& outCursorType) const
{
    if (auto socket = m_hoverSocket.lock())
    {
        if (socket->info().m_canInitializeConnections)
        {
            outCursorType = CursorType::Hand;
            return true;
        }
    }

    return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
}

bool GraphEditor::handleContextMenu(const ElementArea& area, const Position& absolutePosition, InputKeyMask controlKeys)
{
    auto buttons = RefNew<MenuButtonContainer>();

    const auto margin = 20.0f * cachedStyleParams().pixelScale;
    if (auto* contextBlock = rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(absolutePosition, margin)))
    {
        if (auto* contextSocket = contextBlock->socketAtAbsolutePos(absolutePosition))
        {
            buildSocketPopupMenu(contextBlock, const_cast<graph::Socket*>(contextSocket), *buttons);
        }
        else
        {
            buildBlockPopupMenu(contextBlock, *buttons);
        }
    }
    else
    {
        buildGenericPopupMenu(absolutePosition, *buttons);
    }

    buttons->show(this);
    return true;
}
        
void GraphEditor::renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderCustomOverlayElements(hitCache, stash, outerArea, outerClipArea, canvas, mergedOpacity);

    // TODO: culling!!!
    for (auto* node : m_nodesMap.values())
        node->drawConnections(outerArea, outerClipArea, canvas, mergedOpacity);
}

//--

bool GraphEditor::createPreviewNode(const Point& absolutePosition, ClassType blockClass)
{
    removePreviewNode();

    if (auto tempBlock = blockClass.create<graph::Block>())
    {
        // create default sockets
        tempBlock->rebuildLayout();

        // create node visualization
        auto node = RefNew<GraphEditorBlockNode>(tempBlock);
        node->hitTest(false);
        node->opacity(0.5f);
        node->refreshBlockStyle();
        node->refreshBlockSockets();
        node->virtualPosition(absoluteToVirtual(absolutePosition));

        attachChild(node);
        m_previewBlock = node;
        return true;
    }

    return false;
}

void GraphEditor::movePreviewNode(const Point& absolutePosition)
{
    if (m_previewBlock)
    {
        auto snappedPosition = absoluteToVirtual(absolutePosition);
        //snappedPosition.x = std::roundf(snappedPosition.x / m_gridSize) * m_gridSize;
        //snappedPosition.y = std::roundf(snappedPosition.y / m_gridSize) * m_gridSize;
        m_previewBlock->virtualPosition(snappedPosition);
    }
}

void GraphEditor::removePreviewNode()
{
    if (m_previewBlock)
    {
        detachChild(m_previewBlock);
        m_previewBlock.reset();
    }
}

//--

class GraphEditorDragDropPreviewHandler : public IDragDropHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphEditorDragDropPreviewHandler, IDragDropHandler);

public:
    GraphEditorDragDropPreviewHandler(GraphEditor* editor, const DragDropDataPtr& data, const Position& initialPosition, ClassType blockClass)
        : IDragDropHandler(data, editor, initialPosition)
        , m_editor(editor)
        , m_blockClass(blockClass)
    {
    }

    virtual ~GraphEditorDragDropPreviewHandler()
    {
    }

    virtual void handleCancel() override
    {
        removePreviewRepresentation();
    }

    virtual bool canHideDefaultDataPreview() const override final
    {
        return true;
    }

    virtual bool canHandleDataAt(const Position& absolutePos) const override final
    {
        m_editor->movePreviewNode(absolutePos);
        return true;
    }

    virtual void handleData(const Position& absolutePos) const override final
    {
        // remove the temp preview block
        removePreviewRepresentation();

        // create block
        const auto pos = m_editor->absoluteToVirtual(absolutePos);
        m_editor->actionCreateBlock(m_blockClass, pos);

        // focus the editor after D&D so we can delete the wrong node we just dropped :)
        m_editor->focus();
    }

private:
    GraphEditor* m_editor;
    ClassType m_blockClass;

    void removePreviewRepresentation() const
    {
        m_editor->removePreviewNode();
    }
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphEditorDragDropPreviewHandler);
RTTI_END_TYPE();

//--

DragDropHandlerPtr GraphEditor::handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition)
{
    // we can only accept the block class
    auto blockClassData = rtti_cast<GraphBlockClassDragDropData>(data);
    if (!blockClassData)
        return nullptr;

    // check if the block class supported by the graph
    if (!m_graph || !m_graph->canAddBlockOfClass(blockClassData->blockClass()))
        return nullptr;

    // create a preview node
    if (createPreviewNode(entryPosition, blockClassData->blockClass()))
        return RefNew<GraphEditorDragDropPreviewHandler>(this, data, entryPosition, blockClassData->blockClass());

    // no valid drag&drop
    return nullptr;
}

//--

void GraphEditor::cmdPasteAtPoint()
{
    // TODO
}

//--

StringBuf FormatBlockClassDisplayTitle(ClassType blockClass)
{
    if (blockClass)
    {
        if (const auto* groupInfo = blockClass->findMetadata<graph::BlockInfoMetadata>())
        {
            if (groupInfo->nameString)
                return StringBuf(groupInfo->nameString);
            else
                return StringBuf(groupInfo->titleString);
        }
    }

    return StringBuf::EMPTY();
}

StringBuf FormatBlockClassDisplayString(ClassType blockClass, bool includeGroupTag /*= true*/)
{
    if (blockClass)
    {
        if (const auto* groupInfo = blockClass->findMetadata<graph::BlockInfoMetadata>())
        {
            StringView groupName = "Generic";
            if (!groupInfo->groupString.empty())
                groupName = groupInfo->groupString;

            StringView blockName = groupInfo->nameString;
            if (blockName.empty())
                blockName = groupInfo->titleString;

            Color tagColor(0, 0, 0, 0);
            if (const auto* titleColor = blockClass->findMetadata<graph::BlockTitleColorMetadata>())
                tagColor = titleColor->value;

            if (includeGroupTag && tagColor.a > 0)
            {
                return StringBuf(TempString("[img:fx] {} [tag:{}]{}[/tag]", blockName, tagColor.toHexString(), groupName));
            }
            else
            {
                return StringBuf(TempString("[img:fx] {}", blockName));
            }
        }
    }

    return StringBuf::EMPTY();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphBlockClassDragDropData);
RTTI_END_TYPE();

GraphBlockClassDragDropData::GraphBlockClassDragDropData(SpecificClassType<graph::Block> blockClass)
    : DragDropData_String(FormatBlockClassDisplayString(blockClass))
    , m_blockClass(blockClass)
{}

//--

END_BOOMER_NAMESPACE_EX(ui)
