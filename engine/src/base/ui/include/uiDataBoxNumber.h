/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiDataBox.h"

namespace ui
{
    ///---

    /// data box for numbers based on editable text box
    class DataBoxNumberText : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxNumberText, IDataBox);

    public:
        DataBoxNumberText(base::Type type, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool dragger, base::StringView<char> units);

    protected:
        void write();

        virtual void enterEdit() override;
        virtual void cancelEdit() override;
        virtual void handleValueChange() override;

        base::Type m_type;

        bool m_rangeEnabled = false;
        bool m_typeInteger = false;
        bool m_valueReadInRange = false;
        bool m_dragWrap = false;

        int m_numFractionalDigits = 0;
        double m_rangeMin = 0.0;
        double m_rangeMax = 0.0;

        //--

        TextEditorPtr m_editBox;
        DraggerPtr m_dragBox;

        //--

        void dragStart();
        void dragFinish();
        void dragCancel();
        void dragUpdate(int64_t numSteps);

        base::rtti::DataHolder m_preDragValue;
        int64_t m_dragStepCounter;
    };

    ///---

    /// track bar based data box for numbers
    class DataBoxNumberTrackBar : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxNumberTrackBar, IDataBox);

    public:
        DataBoxNumberTrackBar(base::Type type, int numDigits, double rangeMin, double rangeMax, bool editable);

    protected:
        void write();

        virtual void enterEdit() override;
        virtual void cancelEdit() override;
        virtual void handleValueChange() override;

        base::Type m_type;

        bool m_typeInteger = false;
        int m_numFractionalDigits = 0;
        double m_rangeMin = 0.0;
        double m_rangeMax = 0.0;

        //--

        TrackBarPtr m_bar;
    };

    ///---

} // ui