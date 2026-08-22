#include "ShitEngine/Math.h"
#include <SDL3/SDL_rect.h>

namespace Shit {

SDL_FRect Rect::toSDL() const
{
    return { x, y, w, h };
}

Rect Rect::fromSDL(const SDL_FRect &r)
{
    return { r.x, r.y, r.w, r.h };
}

} // namespace Shit
