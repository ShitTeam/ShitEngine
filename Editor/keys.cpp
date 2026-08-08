#include "keys.h"

#include <SDL3/SDL_keyboard.h>  // SDL_GetScancodeName

SDL_Scancode sdlScancodeForQtKey(int qtKey)
{
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (qtKey - Qt::Key_A));
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_0 + (qtKey - Qt::Key_0));
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_F1 + (qtKey - Qt::Key_F1));

    switch (qtKey) {
        case Qt::Key_Space:       return SDL_SCANCODE_SPACE;
        case Qt::Key_Return:      return SDL_SCANCODE_RETURN;
        case Qt::Key_Enter:       return SDL_SCANCODE_RETURN;   // 小键盘回车
        case Qt::Key_Backspace:   return SDL_SCANCODE_BACKSPACE;
        case Qt::Key_Tab:         return SDL_SCANCODE_TAB;
        case Qt::Key_Escape:      return SDL_SCANCODE_ESCAPE;
        case Qt::Key_Delete:      return SDL_SCANCODE_DELETE;
        case Qt::Key_Home:        return SDL_SCANCODE_HOME;
        case Qt::Key_End:         return SDL_SCANCODE_END;
        case Qt::Key_PageUp:      return SDL_SCANCODE_PAGEUP;
        case Qt::Key_PageDown:    return SDL_SCANCODE_PAGEDOWN;
        case Qt::Key_Up:          return SDL_SCANCODE_UP;
        case Qt::Key_Down:        return SDL_SCANCODE_DOWN;
        case Qt::Key_Left:        return SDL_SCANCODE_LEFT;
        case Qt::Key_Right:       return SDL_SCANCODE_RIGHT;
        case Qt::Key_Shift:       return SDL_SCANCODE_LSHIFT;
        case Qt::Key_Control:     return SDL_SCANCODE_LCTRL;
        case Qt::Key_Alt:         return SDL_SCANCODE_LALT;
        case Qt::Key_Meta:        return SDL_SCANCODE_LGUI;
        case Qt::Key_Semicolon:   return SDL_SCANCODE_SEMICOLON;
        case Qt::Key_Apostrophe:  return SDL_SCANCODE_APOSTROPHE;
        case Qt::Key_Comma:       return SDL_SCANCODE_COMMA;
        case Qt::Key_Period:      return SDL_SCANCODE_PERIOD;
        case Qt::Key_Slash:       return SDL_SCANCODE_SLASH;
        case Qt::Key_Backslash:   return SDL_SCANCODE_BACKSLASH;
        case Qt::Key_BracketLeft: return SDL_SCANCODE_LEFTBRACKET;
        case Qt::Key_BracketRight:return SDL_SCANCODE_RIGHTBRACKET;
        case Qt::Key_Minus:       return SDL_SCANCODE_MINUS;
        case Qt::Key_Equal:       return SDL_SCANCODE_EQUALS;
        case Qt::Key_QuoteLeft:   return SDL_SCANCODE_GRAVE;
        default:                  return SDL_SCANCODE_UNKNOWN;
    }
}

QString sdlKeyName(SDL_Scancode sc)
{
    if (sc == SDL_SCANCODE_UNKNOWN || sc >= SDL_SCANCODE_COUNT) return {};
    const char *name = SDL_GetScancodeName(sc);
    return name ? QString::fromUtf8(name) : QString();
}

QString sdlKeyNameForQtKey(int qtKey)
{
    return sdlKeyName(sdlScancodeForQtKey(qtKey));
}

QString mouseBindingName(Qt::MouseButton button)
{
    switch (button) {
        case Qt::LeftButton:   return QStringLiteral("MouseButton.Left");
        case Qt::RightButton:  return QStringLiteral("MouseButton.Right");
        case Qt::MiddleButton: return QStringLiteral("MouseButton.Middle");
        case Qt::XButton1:     return QStringLiteral("MouseButton.XButton1");
        case Qt::XButton2:     return QStringLiteral("MouseButton.XButton2");
        default:               return {};
    }
}