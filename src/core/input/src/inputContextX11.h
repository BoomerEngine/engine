/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\x11 #]
* [# platform: linux #]
***/

#pragma once

#include "inputContext.h"
#include "inputGenericKeyboard.h"
#include "inputGenericMouse.h"
#include "inputStructures.h"

namespace base
{
    namespace input
    {
        class ContextX11;

        //--

        /// X11 based keyboard implementation
        class X11Keyboard : public GenericKeyboard
        {
        public:
            X11Keyboard(IContext* context);

            void nativeKeyPress(uint32_t keyCode);
            void nativeKeyRelease(uint32_t keyCode);
            void nativeKeyType(wchar_t ch);

        private:
            KeyCode m_keyMapping[256];
        };

        //--

        /// X11 mouse implementation
        class X11Mouse : public GenericMouse
        {
        public:
            X11Mouse(ContextX11* context, GenericKeyboard* keyboard);

            void nativeButtonPress(uint32_t buttonIndex, const Point& windowPos, const Point& absolutePos);
            void nativeButtonRelease(uint32_t buttonIndex, const Point& windowPos, const Point& absolutePos);
            void nativeCursorMove(const Point& windowPos, const Point& absolutePos, const Point* knownDelta);
            void nativeWheelMove(const Point& windowPos, const Point& absolutePos, int delta);

        private:
            virtual bool ensureCapture(const Point& windowPoint) override final;
            virtual void releaseCapture() override final;
            virtual bool isCaptured() const override final;

            Point absoluteCursorPos() const;

            ContextX11* m_system;

            void* m_captureDisplay;
            uint64_t m_captureWindow;

            Point m_lastMouseMovePos;
        };

        //--        

        /// input system wrapper for the X11, managed the Window/Input interaction
        class ContextX11 : public IContext
        {
        public:
            ContextX11(uint64_t nativeWindow, uint64_t nativeDisplay);

            INLINE uint64_t window() const { return m_window; }
            INLINE void* display() const { return m_display; }

            virtual void resetInput() override final;
            virtual void processState() override final;
            virtual void processMessage(const void* msg) override final;

        private:
            void processNativeEvent(const NativeEventX11& evt);

            Point absoluteCursorPos(window::IWindow* window) const;

            X11Keyboard m_keyboard;
            X11Mouse m_mouse;

            uint64_t m_window;
            void* m_display;
        };

        } // x11
    } // input
} // base
