#include "app/Window.hpp"

#include <stdexcept>

namespace lof::app {

Window::Window(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(SDL_GetError());
    }
    if (!SDL_CreateWindowAndRenderer(title, width, height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window_,
                                     &renderer_)) {
        const char* msg = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(msg);
    }
    SDL_SetRenderVSync(renderer_, 1);
}

Window::~Window() {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

float Window::pixel_scale() const {
    const float d = SDL_GetWindowPixelDensity(window_);
    return d < 1.0f ? 1.0f : d;
}

bool Window::pump_events() {
    bool running = true;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            running = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
            running = false;
        }
    }
    return running;
}

}  // namespace lof::app
