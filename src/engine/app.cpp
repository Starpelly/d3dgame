#include "app.hpp"
#include "renderer.hpp"

#include <SDL3/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <empscripten/html5.h>
#endif

using namespace Engine;

namespace
{
    // Global app state
    bool app_is_running = false;
    bool app_is_exiting = false;
    SDL_Window* app_window = nullptr;
    Renderer* app_renderer_api = nullptr;
}

bool App::run()
{
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    // Create SDL window
    app_window = SDL_CreateWindow("game", 1280, 720, 0);
    if (!app_window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    app_renderer_api = Renderer::try_make_renderer(RendererType::D3D11);
    app_renderer_api->init();

    app_is_running = true;

    // Begin main loop
    while (!app_is_exiting)
    {
        Internal::app_step();
    }

    return true;
}

bool App::is_running()
{
	return app_is_running;
}

void App::exit()
{
    app_is_running = false;

    app_renderer_api->shutdown();
    delete app_renderer_api;

    SDL_DestroyWindow(app_window);
    SDL_Quit();
}

glm::ivec2 App::get_size()
{
    int x, y;
    SDL_GetWindowSize(app_window, &x, &y);

    return { x, y };
}

void* App::get_window_ptr()
{
    return app_window;
}

void Internal::app_step()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        {
            app_is_exiting = true;
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            auto button = event.button.button - 1;
            if (button == 2) button = 1; 		// Right mouse button = 2
            else if (button == 1) button = 2; 	// Middle mouse button = 2
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            auto button = event.button.button - 1;
            if (button == 2) button = 1;		// Right mouse button = 2
            else if (button == 1) button = 2;	// Middle mouse button = 2
        } break;
        case SDL_EVENT_MOUSE_WHEEL:
        {
            const auto xOffset = static_cast<float>(event.wheel.x);
            const auto yOffset = static_cast<float>(event.wheel.y);
        } break;
        }

        app_renderer_api->before_render();
        app_renderer_api->clear_backbuffer({ 0.392f, 0.584f, 0.929f, 1.0f }, 0, 0);
        app_renderer_api->render({});
        app_renderer_api->after_render();
    }
}
