#include "platform.hpp"
#include "app.hpp"

#include <SDL3/SDL.h>

using namespace Engine;

void* Platform::d3d11_get_hwnd()
{
#if _WIN32
	return SDL_GetPointerProperty(SDL_GetWindowProperties((SDL_Window*)App::get_window_ptr()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#else
	return nullptr;
#endif
}