#include "platform.hpp"

#include <SDL3/SDL.h>

SDL_Window* sdl_window = nullptr;

void* Platform::d3d11_get_hwnd()
{
#if _WIN32
	return SDL_GetPointerProperty(SDL_GetWindowProperties(sdl_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#else
	return nullptr;
#endif
}