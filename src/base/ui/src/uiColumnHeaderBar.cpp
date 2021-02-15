/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiColumnHeaderBar.h"
#include "uiColumnHeaderItem.h"

#include "base/image/include/image.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"

namespace ui
{

    //--

    class ColumnSizeChangeInputAction : public MouseInputAction
    {
    public:
        ColumnSizeChangeInputAction(ColumnHeaderBar* header, int columnIndex, float referencePos, const Position& delta)
            : MouseInputAction(header, base::input::KeyCode::KEY_MOUSE0)
            , m_header(header)
            , m_referencePos(referencePos)
            , m_columnIndex(columnIndex)
            , m_delta(delta)
        {
        }

        virtual ~ColumnSizeChangeInputAction()
        {
        }

        virtual void onCanceled() override
        {
        }

        virtual void onFinished()  override
        {
        }

        virtual bool allowsHoverTracking() const override
        {
            return false;
        }

        virtual void onUpdateCursor(base::input::CursorType& outCursorType) override
        {
            outCursorType = base::input::CursorType::SizeWE;
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (auto header = m_header.lock())
            {
                auto unadjustedWidth = std::max<float>(evt.absolutePosition().x - m_delta.x - m_referencePos, 0.0f);
                auto adjustedWidth = unadjustedWidth / header->cachedStyleParams().pixelScale;
                header->adjustColumnWidthInternal(m_columnIndex, adjustedWidth);
            }

            return InputActionResult();
        }

    private:
        base::RefWeakPtr<ColumnHeaderBar> m_header;
        float m_referencePos;
        int m_columnIndex;
        Position m_delta;
    };

    //--

    RTTI_BEGIN_TYPE_CLASS(ColumnHeaderBar);
        RTTI_METADATA(ElementClassNameMetadata).name("ColumnHeaderBar");
    RTTI_END_TYPE();

    ColumnHeaderBar::ColumnHeaderBar()
        : m_sortingColumn(-1)
        , m_sortingAscending(true)
    {
        m_splitterSnapDistance = 5.0f;

        layoutMode(LayoutMode::Horizontal);
        hitTest(HitTestState::Enabled);
        dynamicSizingData(true);
        hitTest(true);
    }

    bool ColumnHeaderBar::handleTemplateProperty(base::StringView name, base::StringView value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
    }

    void ColumnHeaderBar::addColumn(base::StringView caption, float width, bool center /*= false*/, bool allowSort /*= true*/, bool allowResize /*= true*/)
    {
        auto& entry = m_columns.emplaceBack();
        entry.m_currentWidth = width;
        entry.m_minWidth = std::min<float>(100.0f, width);
        entry.m_maxWidth = std::max<float>(600.0f, width * 4.0f);
        entry.m_canSort = allowSort;
        entry.m_canResize = allowResize;
        entry.m_center = center;
        entry.m_header = base::RefNew<ColumnHeader>(caption, entry.m_canResize, entry.m_canSort);

        if (entry.m_canSort)
        {
            entry.m_header->bind(EVENT_CLICKED) = [this](ColumnHeader* button)
            {
                auto index = findColumnIndex(button);
                sort(index, (index == m_sortingColumn) ? !m_sortingAscending : true);
            };
        }

        entry.m_header->customMinSize(Size(entry.m_currentWidth, 0.0f));
        entry.m_header->customMaxSize(Size(entry.m_currentWidth, 0.0f));
        attachChild(entry.m_header);
    }

    bool ColumnHeaderBar::handleTemplateChild(base::StringView name, const base::xml::IDocument& doc, const base::xml::NodeID& id)
    {
        if (name == "Column" || name == "ColumnHeader")
        {
            auto caption = doc.nodeAttributeOfDefault(id, "text");
            if (!caption)
                return false;

            auto& entry = m_columns.emplaceBack();
            /*if (icon)
            {
                auto iconFile = 
                entry.m_header = base::RefNew<ColumnHeader>("[icon]");// iconFile);
                entry.m_currentWidth = 0.0f;
                entry.m_canSort = false;
                entry.m_canResize = false;
                entry.m_center = true;
            }
            else*/
            {
                doc.nodeAttributeOfDefault(id, "width").match(entry.m_currentWidth);
                doc.nodeAttributeOfDefault(id, "minWidth").match(entry.m_minWidth);
                doc.nodeAttributeOfDefault(id, "maxWidth").match(entry.m_maxWidth);
                doc.nodeAttributeOfDefault(id, "sort").match(entry.m_canSort);
                doc.nodeAttributeOfDefault(id, "resize").match(entry.m_canResize);
                doc.nodeAttributeOfDefault(id, "center").match(entry.m_center);
                entry.m_header = base::RefNew<ColumnHeader>(caption, entry.m_canResize, entry.m_canSort);

                if (entry.m_canSort)
                {
                    entry.m_header->bind(EVENT_CLICKED) = [this](ColumnHeader* button)
                    {
                        auto index = findColumnIndex(button);
                        sort(index, (index == m_sortingColumn) ? !m_sortingAscending : true);
                    };
                }

                entry.m_header->customMinSize(Size(entry.m_currentWidth, 0.0f));
                entry.m_header->customMaxSize(Size(entry.m_currentWidth, 0.0f));
            }

            attachChild(entry.m_header);
            return true;
        }

        return TBaseClass::handleTemplateChild(name, doc, id);
    }

    bool ColumnHeaderBar::handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement)
    {
        return false;
    }

    bool ColumnHeaderBar::handleTemplateFinalize()
    {
        return TBaseClass::handleTemplateFinalize();
    }

    //--

    void ColumnHeaderBar::prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const
    {
        // cache sizes
        m_cachedColumnAreas.reset();
        m_cachedColumnAreas.reserve(m_columns.size());
        for (const auto& entry : m_columns)
        {
            auto& info = m_cachedColumnAreas.emplaceBack();
            info.m_left = entry.m_header->cachedDrawArea().left();
            info.m_right = entry.m_header->cachedDrawArea().right();
            info.m_center = entry.m_center;
        }

        m_cachedSizingData.m_numColumns = m_cachedColumnAreas.size();
        m_cachedSizingData.m_columns = m_cachedColumnAreas.typedData();

        dataPtr = &m_cachedSizingData;
    }

    InputActionPtr ColumnHeaderBar::handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        // start column size dragging if the column supports resizing
        if (evt.leftClicked())
        {
            auto columnIndex = findSplitterColumnIndex(area, Position((float)evt.absolutePosition().x, (float)evt.absolutePosition().y));
            if (columnIndex != -1)
            {
                const auto& info = m_columns[columnIndex];
                if (info.m_canResize)
                {
                    auto columnBase = info.m_header->cachedDrawArea().left();
                    auto columnSplitter = info.m_header->cachedDrawArea().right();

                    auto action = base::RefNew<ColumnSizeChangeInputAction>(this, columnIndex, columnBase, Position((float)evt.absolutePosition().x - columnSplitter, 0));
                    return action;
                }
            }
        }

        // pass to base class
        return TBaseClass::handleOverlayMouseClick(area, evt);
    }

    void ColumnHeaderBar::sort(int columnIndex, bool asc /*= true*/, bool callEvent /*= true*/)
    {
        if (m_sortingColumn != columnIndex || m_sortingAscending != asc)
        {
            m_sortingColumn = columnIndex;
            m_sortingAscending = asc;

            for (uint32_t i=0; i<m_columns.size(); ++i)
            {
                const auto &info = m_columns[i];
                info.m_header->refreshSortingVisualization(i == m_sortingColumn, m_sortingAscending);
            }

            //if (callEvent && OnSortingChanged)
              //  OnSortingChanged(this, nullptr);
        }
    }

    void ColumnHeaderBar::adjustColumnWidthInternal(int columnIndex, float pos)
    {
        if (columnIndex >= 0 && columnIndex <= m_columns.lastValidIndex())
        {
            auto& info = m_columns[columnIndex];

            auto newWidth = pos;
            if (newWidth < info.m_minWidth)
                newWidth = info.m_minWidth;
            if (info.m_maxWidth && newWidth > info.m_maxWidth)
                newWidth = info.m_maxWidth;

            if (newWidth != info.m_currentWidth)
            {
                info.m_currentWidth = newWidth;
                info.m_header->customMinSize(Size(info.m_currentWidth, 0.0f));
                info.m_header->customMaxSize(Size(info.m_currentWidth, 0.0f));
            }
        }
    }

    int ColumnHeaderBar::findColumnIndex(IElement* ptr) const
    {
        for (uint32_t i=0; i<m_columns.size(); ++i)
        {
            const auto &info = m_columns[i];
            if (info.m_header == ptr)
                return i;
        }

        return INDEX_NONE;
    }

    int ColumnHeaderBar::findSplitterColumnIndex(const ElementArea& area, const Position& absolutePosition) const
    {
        // find the splitter we may be neer
        for (uint32_t i=0; i<m_columns.size(); ++i)
        {
            const auto& info = m_columns[i];
            if (info.m_header)
            {
                const auto &area = info.m_header->cachedDrawArea();
                auto splitterPos = area.right();

                if (fabs(splitterPos - absolutePosition.x) < m_splitterSnapDistance)
                    return i;
            }
        }

        // no column selected
        return -1;
    }

    bool ColumnHeaderBar::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        // check if we are near the splitter
        auto columnIndex = findSplitterColumnIndex(area, absolutePosition);
        if (columnIndex != -1)
        {
            const auto &info = m_columns[columnIndex];
            if (info.m_canResize)
            {
                outCursorType = base::input::CursorType::SizeWE;
                return true;
            }
        }

        // pass to base class
        return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
    }

} // ui