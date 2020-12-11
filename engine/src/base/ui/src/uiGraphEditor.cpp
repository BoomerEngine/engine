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

#include "base/canvas/include/canvas.h"
#include "base/image/include/image.h"
#include "base/graph/include/graphContainer.h"
#include "base/graph/include/graphSocket.h"
#include "base/graph/include/graphBlock.h"
#include "base/object/include/actionHistory.h"
#include "base/canvas/include/canvasGeometryBuilder.h"

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_CLASS(GraphEditor);
        RTTI_METADATA(ElementClassNameMetadata).name("GraphEditor");
    RTTI_END_TYPE();

    GraphEditor::GraphEditor()
    {
        hitTest(HitTestState::Enabled);
        allowFocusFromKeyboard(true);

        actions().bindCommand("GraphEditor.CopySelection"_id) = [this]() { actionCopySelection(); };
        actions().bindCommand("GraphEditor.CutSelection"_id) = [this]() { actionCutSelection(); };
        actions().bindCommand("GraphEditor.PasteSelection"_id) = [this]() { actionPasteSelection(false, VirtualPosition(16,16)); };
        actions().bindCommand("GraphEditor.DeleteSelection"_id) = [this]() { actionDeleteSelection(); };

        actions().bindShortcut("GraphEditor.CopySelection"_id, "Ctrl+C");
        actions().bindShortcut("GraphEditor.CutSelection"_id, "Ctrl+X");
        actions().bindShortcut("GraphEditor.PasteSelection"_id, "Ctrl+V");
        actions().bindShortcut("GraphEditor.DeleteSelection"_id, "Delete");
    }

    GraphEditor::~GraphEditor()
    {
        deleteVisualizations();
    }

    void GraphEditor::bindGraph(const base::graph::ContainerPtr& graph)
    {
        if (m_graph != graph)
        {
            deleteVisualizations();
            m_graph = graph;
            createVisualizations();
        }
    }

    void GraphEditor::bindActionHistory(const base::ActionHistoryPtr& actionHistory)
    {
        m_actionHistory = actionHistory;
    }

    const base::graph::Socket* GraphEditor::findSocketAtAbsolutePos(const base::Point& pos) const
    {
        const auto margin = 20.0f * cachedStyleParams().pixelScale;
        auto* newBlock = base::rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(pos, margin));
        return newBlock ? newBlock->socketAtAbsolutePos(pos.toVector()) : nullptr;
    }

    bool GraphEditor::calcSocketLinkPlacement(const base::graph::Socket* socket, Position& outSocketAbsolutePosition, base::Vector2& outSocketDirection) const
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

    void GraphEditor::enumerateSelectedBlocks(const std::function<bool(base::graph::Block*)>& enumFunc) const
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

    void GraphEditor::handleBlockAdded(base::graph::Block* block)
    {
        DEBUG_CHECK_EX(!m_nodesMap.contains(block), "Block already seen");
        if (!m_nodesMap.contains(block))
        {
            auto node = base::RefNew<GraphEditorBlockNode>(block);
            //node->parent(this);
            node->refreshBlockStyle();

            m_nodes.pushBack(node);
            m_nodesMap[block] = node;
            attachChild(node);
        }
    }

    void GraphEditor::handleBlockRemoved(base::graph::Block* block)
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

    void GraphEditor::handleBlockStyleChanged(base::graph::Block* block)
    {
        if (auto node = m_nodesMap.findSafe(block, nullptr))
            node->refreshBlockStyle();

    }

    void GraphEditor::handleBlockLayoutChanged(base::graph::Block* block)
    {
        if (auto node = m_nodesMap.findSafe(block, nullptr))
            node->refreshBlockSockets();
    }

    void GraphEditor::handleBlockConnectionsChanged(base::graph::Block* block)
    {

    }

    bool GraphEditor::handleMouseMovement(const base::input::MouseMovementEvent& evt)
    {
        refreshHoverFromPosition(evt.absolutePosition().toVector());
        return TBaseClass::handleMouseMovement(evt);
    }

    void GraphEditor::updateHover(GraphEditorBlockNode* block, const base::graph::Socket* socket)
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
        auto* newBlock = base::rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(pos, margin));
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

    void GenerateConnectionGeometry(const Position& sourcePos, const base::Vector2& sourceDir, const Position& targetPos, const base::Vector2& targetDir, base::Color linkColor, float width, float pixelScale, bool startArrow, bool endArrow, base::canvas::GeometryBuilder& builder)
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
            GraphCreateConnectionInputAction(GraphEditor* editor, base::RefWeakPtr<base::graph::Socket> sourceSocket, const base::Point& initialPoint)
                : MouseInputAction(editor, base::input::KeyCode::KEY_MOUSE0)
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

            virtual void onUpdateCursor(base::input::CursorType& outCursorType) override
            {
                if (auto target = m_foundTargetSocket.lock())
                {
                    if (m_foundTargetSocketValid)
                        outCursorType = base::input::CursorType::Hand;
                    else
                        outCursorType = base::input::CursorType::No;
                }
                else
                {
                    outCursorType = base::input::CursorType::Default;
                }
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
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

            virtual void onRender(base::canvas::Canvas& canvas) override
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
							base::canvas::Geometry geometry;

							{
								base::canvas::GeometryBuilder builder(nullptr, geometry);

								const auto pixelWidth = m_editor->cachedStyleParams().pixelScale;
								GenerateConnectionGeometry(sourcePos, sourceDir, targetPos, targetDir, validSourceSocket->info().m_linkColor, 4.0f, pixelWidth, false, false, builder);
							}

							canvas.place(base::canvas::Placement(), geometry);
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
                            m_foundTargetSocketValid = base::graph::Socket::CanConnect(*validSourceSocket, *validDestDocket);
                        }
                    }                    
                }
            }

        private:
            GraphEditor* m_editor;
            base::Point m_lastAbsolutePoint;

            base::RefWeakPtr<base::graph::Socket> m_sourceSocket = nullptr;
            base::RefWeakPtr<base::graph::Socket> m_foundTargetSocket = nullptr;
            bool m_foundTargetSocketValid = false;
        };

    } // prv

    InputActionPtr GraphEditor::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked())
            if (m_hoverBlock && m_hoverSocket)
                return base::RefNew<prv::GraphCreateConnectionInputAction>(this, m_hoverSocket, evt.absolutePosition());

        return TBaseClass::handleMouseClick(area, evt);
    }

    InputActionPtr GraphEditor::handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (m_hoverBlock && m_hoverSocket)
            return InputActionPtr();

        return TBaseClass::handleOverlayMouseClick(area, evt);
    }

    bool GraphEditor::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        if (auto socket = m_hoverSocket.lock())
        {
            if (socket->info().m_canInitializeConnections)
            {
                outCursorType = base::input::CursorType::Hand;
                return true;
            }
        }

        return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
    }

    bool GraphEditor::handleContextMenu(const ElementArea& area, const Position& absolutePosition)
    {
        auto buttons = base::RefNew<MenuButtonContainer>();

        const auto margin = 20.0f * cachedStyleParams().pixelScale;
        if (auto* contextBlock = base::rtti_cast<GraphEditorBlockNode>(findElementAtAbsolutePos(absolutePosition, margin)))
        {
            if (auto* contextSocket = contextBlock->socketAtAbsolutePos(absolutePosition))
            {
                buildSocketPopupMenu(contextBlock, const_cast<base::graph::Socket*>(contextSocket), *buttons);
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
        
    void GraphEditor::renderCustomOverlayElements(HitCache& hitCache, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderCustomOverlayElements(hitCache, outerArea, outerClipArea, canvas, mergedOpacity);

        // TODO: culling!!!
        for (auto* node : m_nodesMap.values())
            node->drawConnections(outerArea, outerClipArea, canvas, mergedOpacity);
    }

    //--

    bool GraphEditor::createPreviewNode(const base::Point& absolutePosition, base::ClassType blockClass)
    {
        removePreviewNode();

        if (auto tempBlock = blockClass.create<base::graph::Block>())
        {
            // create default sockets
            tempBlock->rebuildLayout();

            // create node visualization
            auto node = base::RefNew<GraphEditorBlockNode>(tempBlock);
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

    void GraphEditor::movePreviewNode(const base::Point& absolutePosition)
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
        GraphEditorDragDropPreviewHandler(GraphEditor* editor, const DragDropDataPtr& data, const Position& initialPosition, base::ClassType blockClass)
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
        base::ClassType m_blockClass;

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
        auto blockClassData = base::rtti_cast<GraphBlockClassDragDropData>(data);
        if (!blockClassData)
            return nullptr;

        // check if the block class supported by the graph
        if (!m_graph || !m_graph->canAddBlockOfClass(blockClassData->blockClass()))
            return nullptr;

        // create a preview node
        if (createPreviewNode(entryPosition, blockClassData->blockClass()))
            return base::RefNew<GraphEditorDragDropPreviewHandler>(this, data, entryPosition, blockClassData->blockClass());

        // no valid drag&drop
        return nullptr;
    }

    //--

    void GraphEditor::cmdPasteAtPoint()
    {
        // TODO
    }

    //--

    base::StringBuf FormatBlockClassDisplayTitle(base::ClassType blockClass)
    {
        if (blockClass)
        {
            if (const auto* groupInfo = blockClass->findMetadata<base::graph::BlockInfoMetadata>())
            {
                if (groupInfo->nameString)
                    return base::StringBuf(groupInfo->nameString);
                else
                    return base::StringBuf(groupInfo->titleString);
            }
        }

        return base::StringBuf::EMPTY();
    }

    base::StringBuf FormatBlockClassDisplayString(base::ClassType blockClass, bool includeGroupTag /*= true*/)
    {
        if (blockClass)
        {
            if (const auto* groupInfo = blockClass->findMetadata<base::graph::BlockInfoMetadata>())
            {
                base::StringView groupName = "Generic";
                if (!groupInfo->groupString.empty())
                    groupName = groupInfo->groupString;

                base::StringView blockName = groupInfo->nameString;
                if (blockName.empty())
                    blockName = groupInfo->titleString;

                base::Color tagColor(0, 0, 0, 0);
                if (const auto* titleColor = blockClass->findMetadata<base::graph::BlockTitleColorMetadata>())
                    tagColor = titleColor->value;

                if (includeGroupTag && tagColor.a > 0)
                {
                    return base::StringBuf(base::TempString("[img:fx] {} [tag:{}]{}[/tag]", blockName, tagColor.toHexString(), groupName));
                }
                else
                {
                    return base::StringBuf(base::TempString("[img:fx] {}", blockName));
                }
            }
        }

        return base::StringBuf::EMPTY();
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphBlockClassDragDropData);
    RTTI_END_TYPE();

    GraphBlockClassDragDropData::GraphBlockClassDragDropData(base::SpecificClassType<base::graph::Block> blockClass)
        : DragDropData_String(FormatBlockClassDisplayString(blockClass))
        , m_blockClass(blockClass)
    {}

    //--

} // ui
