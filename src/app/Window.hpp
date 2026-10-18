#pragma once
#include <SDL3/SDL.h>

namespace lof::app {

// RAII wrapper: SDL init, window, renderer, event pump.
class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Renderer* renderer() const { return renderer_; }

    // Display pixel density (2.0 on Retina). Never below 1.
    float pixel_scale() const;

    // Returns false once the user asked to quit (window close or Esc).
    bool pump_events();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace lof::app
