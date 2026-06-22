#pragma once

#include <SDL3/SDL_scancode.h>

namespace Shit {

    // 按键码 （扫描码）
    enum class KeyCode : int {
        Unknown = SDL_SCANCODE_UNKNOWN,

        // Letters
        A = SDL_SCANCODE_A,
        B = SDL_SCANCODE_B,
        C = SDL_SCANCODE_C,
        D = SDL_SCANCODE_D,
        E = SDL_SCANCODE_E,
        F = SDL_SCANCODE_F,
        G = SDL_SCANCODE_G,
        H = SDL_SCANCODE_H,
        I = SDL_SCANCODE_I,
        J = SDL_SCANCODE_J,
        K = SDL_SCANCODE_K,
        L = SDL_SCANCODE_L,
        M = SDL_SCANCODE_M,
        N = SDL_SCANCODE_N,
        O = SDL_SCANCODE_O,
        P = SDL_SCANCODE_P,
        Q = SDL_SCANCODE_Q,
        R = SDL_SCANCODE_R,
        S = SDL_SCANCODE_S,
        T = SDL_SCANCODE_T,
        U = SDL_SCANCODE_U,
        V = SDL_SCANCODE_V,
        W = SDL_SCANCODE_W,
        X = SDL_SCANCODE_X,
        Y = SDL_SCANCODE_Y,
        Z = SDL_SCANCODE_Z,

        // Numbers (top row)
        Num1 = SDL_SCANCODE_1,
        Num2 = SDL_SCANCODE_2,
        Num3 = SDL_SCANCODE_3,
        Num4 = SDL_SCANCODE_4,
        Num5 = SDL_SCANCODE_5,
        Num6 = SDL_SCANCODE_6,
        Num7 = SDL_SCANCODE_7,
        Num8 = SDL_SCANCODE_8,
        Num9 = SDL_SCANCODE_9,
        Num0 = SDL_SCANCODE_0,

        // Whitespace / control
        Return = SDL_SCANCODE_RETURN,
        Escape = SDL_SCANCODE_ESCAPE,
        Backspace = SDL_SCANCODE_BACKSPACE,
        Tab = SDL_SCANCODE_TAB,
        Space = SDL_SCANCODE_SPACE,

        // Punctuation
        Minus = SDL_SCANCODE_MINUS,
        Equals = SDL_SCANCODE_EQUALS,
        LeftBracket = SDL_SCANCODE_LEFTBRACKET,
        RightBracket = SDL_SCANCODE_RIGHTBRACKET,
        Backslash = SDL_SCANCODE_BACKSLASH,
        NonUSH = SDL_SCANCODE_NONUSHASH,
        Semicolon = SDL_SCANCODE_SEMICOLON,
        Apostrophe = SDL_SCANCODE_APOSTROPHE,
        Grave = SDL_SCANCODE_GRAVE,
        Comma = SDL_SCANCODE_COMMA,
        Period = SDL_SCANCODE_PERIOD,
        Slash = SDL_SCANCODE_SLASH,

        CapsLock = SDL_SCANCODE_CAPSLOCK,

        // Function keys
        F1 = SDL_SCANCODE_F1,
        F2 = SDL_SCANCODE_F2,
        F3 = SDL_SCANCODE_F3,
        F4 = SDL_SCANCODE_F4,
        F5 = SDL_SCANCODE_F5,
        F6 = SDL_SCANCODE_F6,
        F7 = SDL_SCANCODE_F7,
        F8 = SDL_SCANCODE_F8,
        F9 = SDL_SCANCODE_F9,
        F10 = SDL_SCANCODE_F10,
        F11 = SDL_SCANCODE_F11,
        F12 = SDL_SCANCODE_F12,

        PrintScreen = SDL_SCANCODE_PRINTSCREEN,
        ScrollLock = SDL_SCANCODE_SCROLLLOCK,
        Pause = SDL_SCANCODE_PAUSE,
        Insert = SDL_SCANCODE_INSERT,
        Home = SDL_SCANCODE_HOME,
        PageUp = SDL_SCANCODE_PAGEUP,
        Delete = SDL_SCANCODE_DELETE,
        End = SDL_SCANCODE_END,
        PageDown = SDL_SCANCODE_PAGEDOWN,
        Right = SDL_SCANCODE_RIGHT,
        Left = SDL_SCANCODE_LEFT,
        Down = SDL_SCANCODE_DOWN,
        Up = SDL_SCANCODE_UP,

        NumLockClear = SDL_SCANCODE_NUMLOCKCLEAR,

        // Keypad
        KP_Divide = SDL_SCANCODE_KP_DIVIDE,
        KP_Multiply = SDL_SCANCODE_KP_MULTIPLY,
        KP_Minus = SDL_SCANCODE_KP_MINUS,
        KP_Plus = SDL_SCANCODE_KP_PLUS,
        KP_Enter = SDL_SCANCODE_KP_ENTER,
        KP_1 = SDL_SCANCODE_KP_1,
        KP_2 = SDL_SCANCODE_KP_2,
        KP_3 = SDL_SCANCODE_KP_3,
        KP_4 = SDL_SCANCODE_KP_4,
        KP_5 = SDL_SCANCODE_KP_5,
        KP_6 = SDL_SCANCODE_KP_6,
        KP_7 = SDL_SCANCODE_KP_7,
        KP_8 = SDL_SCANCODE_KP_8,
        KP_9 = SDL_SCANCODE_KP_9,
        KP_0 = SDL_SCANCODE_KP_0,
        KP_Period = SDL_SCANCODE_KP_PERIOD,

        NonUSBackslash = SDL_SCANCODE_NONUSBACKSLASH,
        Application = SDL_SCANCODE_APPLICATION,
        Power = SDL_SCANCODE_POWER,
        KP_Equals = SDL_SCANCODE_KP_EQUALS,

        // Extended function keys
        F13 = SDL_SCANCODE_F13,
        F14 = SDL_SCANCODE_F14,
        F15 = SDL_SCANCODE_F15,
        F16 = SDL_SCANCODE_F16,
        F17 = SDL_SCANCODE_F17,
        F18 = SDL_SCANCODE_F18,
        F19 = SDL_SCANCODE_F19,
        F20 = SDL_SCANCODE_F20,
        F21 = SDL_SCANCODE_F21,
        F22 = SDL_SCANCODE_F22,
        F23 = SDL_SCANCODE_F23,
        F24 = SDL_SCANCODE_F24,

        // International / multimedia / mobile keys (partial)
        Sleep = SDL_SCANCODE_SLEEP,
        Wake = SDL_SCANCODE_WAKE,
        MediaPlay = SDL_SCANCODE_MEDIA_PLAY,
        MediaPause = SDL_SCANCODE_MEDIA_PAUSE,
        MediaRecord = SDL_SCANCODE_MEDIA_RECORD,
        MediaNext = SDL_SCANCODE_MEDIA_NEXT_TRACK,
        MediaPrev = SDL_SCANCODE_MEDIA_PREVIOUS_TRACK,
        MediaStop = SDL_SCANCODE_MEDIA_STOP,
        MediaPlayPause = SDL_SCANCODE_MEDIA_PLAY_PAUSE,

        // Modifiers
        LCtrl = SDL_SCANCODE_LCTRL,
        LShift = SDL_SCANCODE_LSHIFT,
        LAlt = SDL_SCANCODE_LALT,
        LGui = SDL_SCANCODE_LGUI,
        RCtrl = SDL_SCANCODE_RCTRL,
        RShift = SDL_SCANCODE_RSHIFT,
        RAlt = SDL_SCANCODE_RALT,
        RGui = SDL_SCANCODE_RGUI,

        Mode = SDL_SCANCODE_MODE,

        // Reserved / bounds
        Reserved = SDL_SCANCODE_RESERVED,
        Count = SDL_SCANCODE_COUNT
    };

    // 鼠标按键码
    enum class MouseButton : int {
        Left = SDL_BUTTON_LEFT,
        Right = SDL_BUTTON_RIGHT,
        Middle = SDL_BUTTON_MIDDLE,
        XButton1 = SDL_BUTTON_X1,
        XButton2 = SDL_BUTTON_X2,
        Count = 5
    };

}