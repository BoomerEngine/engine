/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\dragdrop #]
***/

#pragma once

namespace ui
{

    ///--

    /// drag&drop data
    class BASE_UI_API IDragDropData : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDragDropData);

    public:
        IDragDropData();
        virtual ~IDragDropData();

        /// create the preview of the drag data
        virtual ElementPtr createPreview() const;
    };

    ///--

    /// string based drag&drop data
    class BASE_UI_API DragDropData_String : public IDragDropData
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DragDropData_String, IDragDropData);

    public:
        DragDropData_String(const base::StringBuf& data);

        /// create the preview of the drag data - a simple string in this case
        virtual ElementPtr createPreview() const;

    private:
        base::StringBuf m_data;
    };

    ///--

    /// drag&drop handler, created by the drag&drop targets to handle the pending drag&drop
    class BASE_UI_API IDragDropHandler : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDragDropHandler);

    public:
        IDragDropHandler(const DragDropDataPtr& data, IElement* target, const Position& initialPosition);
        virtual ~IDragDropHandler();

        //--

        // get the dragged data that we have agreed to handle somehow
        INLINE const DragDropDataPtr& data() const { return m_data; }

        // get the drag&drop target element
        // NOTE: the element may get detached during the drag&drop operation
        INLINE IElement* target() const { return m_target.unsafe(); }

        // get the initial position this drag&drop handler was created with
        INLINE const Position& initialPosition() const { return m_initialPosition; }

        // get the last position this drag&drop handler was updated with
        // NOTE: it's at least the initial position
        // NOTE: the position may not be valid to complete the drag&drop
        INLINE const Position& lastPosition() const { return m_lastPosition; }

        // get the last valid position for this drag&drop
        // NOTE: it's at least the initial position
        INLINE const Position& lastValidPosition() const { return m_lastValidPosition; }

        //--

        /// dragging data onto target element was canceled either by entering other element or the user
        /// NOTE: this is called just before releasing the object, should be used to cleanup any visualization at the target
        virtual void handleCancel();

        /// called when the user releases the drag button finally dropping the data onto target element
        /// NOTE: called only if the canHandleDataAt passes, otherwisethe handleCancel is called
        /// NOTE: this is the only required function to be implemented
        virtual void handleData(const Position& absolutePos) const = 0;

        /// test the sub-region of the target element for acceptance/rejection of the drag data
        /// NOTE: in most of the cases we assume that if the drag data is accepted by the item it's accepted anywhere, this can refine the test
        virtual bool canHandleDataAt(const Position& absolutePos) const;

        /// check if should display the original data preview or not
        /// NOTE: usually the data preview is enough but sometimes the target wants to display a custom preview for the dragged data
        virtual bool canHideDefaultDataPreview() const;

        //

        //--

        enum class UpdateResult
        {
            Valid, // the target position is valid for the drag&drop operation with given data in given target
            Invalid, // the target position is not valid for the drag&drop operation with given data in given target but we may continue dragging
            Cancel, // we should cancel the whole drag operation
        };

        // process mouse movement
        UpdateResult processMouseMovement(const base::input::MouseMovementEvent& evt);

        // process mouse wheel
        virtual void handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta);

    private:
        DragDropDataPtr m_data;
        ElementWeakPtr m_target;
        Position m_initialPosition;
        Position m_lastPosition;
        Position m_lastValidPosition;
    };

    //--

    /// generic drag&drop handler for objects that accept the drag&drop data anywhere inside their area and don't require internal preview
    class BASE_UI_API DragDropHandlerGeneric : public IDragDropHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DragDropHandlerGeneric, IDragDropHandler);

    public:
        DragDropHandlerGeneric(const DragDropDataPtr& data, IElement* target, const Position& initialPosition);

        /// IDragDropHandler
        virtual void handleCancel() override;
        virtual void handleData(const Position& absolutePos) const override;
    };

    //--

} // ui