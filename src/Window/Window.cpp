#include "Window.h"

#include <iostream>

namespace gbe {
    
    void* Window::Get_implemented_window() {
        return this->implemented_window;
    }

    Window::~Window() {
        SDL_Quit();
    }

    Window::Window() {
    }

    gbe::Window::Window(Vector2Int dimentions)
    {
        this->dimentions = dimentions; 
        this->viewport_subscale = Vector2(1.0f, 1.0f);

        // Initialize SDL 
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
            throw "Couldn't initialize SDL";
        atexit(SDL_Quit);
        SDL_Vulkan_LoadLibrary(NULL);

        implemented_window = SDL_CreateWindow("GabEngine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dimentions.x, dimentions.y, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        for (size_t i = 0; i < gbe::Keys::_TOTALKEYS; i++)
        {
            this->keystates.insert_or_assign(i, false);
        }
    }

    bool gbe::Window::PollEvents(gbe::window::WindowEventType& event_type) {
        if (this->event_queue.empty())
            return false;

        event_type = this->event_queue.front();
        this->event_queue.pop();
        return true;
    }

    void gbe::Window::UpdateState()
    {
        //States to reset
        this->keystates[gbe::Keys::MOUSE_SCROLL_UP] = false;
        this->keystates[gbe::Keys::MOUSE_SCROLL_DOWN] = false;

        //Handle events on queue
        SDL_Event sdlevent;
        while (SDL_PollEvent(&sdlevent) != 0)
        {
            //User requests quit
            if (sdlevent.type == SDL_QUIT)
            {
                shouldclose = true;
            }
            if (sdlevent.type == SDL_WINDOWEVENT) {
                if (sdlevent.window.event == SDL_WINDOWEVENT_RESIZED) {
                    this->dimentions = Vector2Int(sdlevent.window.data1, sdlevent.window.data2);
                    this->event_queue.push(window::WindowEventType::RESIZE);
                }
            }
            if (sdlevent.type == SDL_MOUSEBUTTONDOWN) {
                if (sdlevent.button.button == SDL_BUTTON_LEFT)
                    this->keystates[gbe::Keys::MOUSE_LEFT] = true;
                if (sdlevent.button.button == SDL_BUTTON_RIGHT)
                    this->keystates[gbe::Keys::MOUSE_RIGHT] = true;
                if (sdlevent.button.button == SDL_BUTTON_MIDDLE)
                    this->keystates[gbe::Keys::MOUSE_MIDDLE] = true;

                std::cout << "SDL_MOUSEBUTTONDOWN" << std::endl;
            }
            if (sdlevent.type == SDL_MOUSEBUTTONUP) {
                if (sdlevent.button.button == SDL_BUTTON_LEFT)
                    this->keystates[gbe::Keys::MOUSE_LEFT] = false;
                if (sdlevent.button.button == SDL_BUTTON_RIGHT)
                    this->keystates[gbe::Keys::MOUSE_RIGHT] = false;
                if (sdlevent.button.button == SDL_BUTTON_MIDDLE)
                    this->keystates[gbe::Keys::MOUSE_MIDDLE] = false;

                std::cout << "SDL_MOUSEBUTTONUP" << std::endl;
            }
            if (sdlevent.type == SDL_MOUSEWHEEL) {
                if (sdlevent.wheel.y > 0)
                    this->keystates[gbe::Keys::MOUSE_SCROLL_UP] = true;
                if(sdlevent.wheel.y < 0)
                    this->keystates[gbe::Keys::MOUSE_SCROLL_DOWN] = true;
            }
            if (sdlevent.type == SDL_KEYDOWN) {
                auto symcode = sdlevent.key.keysym.sym;
                if (symcode == 'w') this->keystates[gbe::Keys::W] = true;
                if (symcode == 'a') this->keystates[gbe::Keys::A] = true;
                if (symcode == 's') this->keystates[gbe::Keys::S] = true;
                if (symcode == 'd') this->keystates[gbe::Keys::D] = true;

                if (symcode == SDLK_SPACE) this->keystates[gbe::Keys::SPACE] = true;
                if (symcode == SDLK_ESCAPE) this->keystates[gbe::Keys::ESCAPE] = true;

                std::cout << "SDL_KEYDOWN" << std::endl;
            }
            if (sdlevent.type == SDL_KEYUP) {
                auto symcode = sdlevent.key.keysym.sym;
                if (symcode == 'w') this->keystates[gbe::Keys::W] = false;
                if (symcode == 'a') this->keystates[gbe::Keys::A] = false;
                if (symcode == 's') this->keystates[gbe::Keys::S] = false;
                if (symcode == 'd') this->keystates[gbe::Keys::D] = false;

                if (symcode == SDLK_SPACE) this->keystates[gbe::Keys::SPACE] = false;
                if (symcode == SDLK_ESCAPE) this->keystates[gbe::Keys::ESCAPE] = false;

                std::cout << "SDL_KEYUP" << std::endl;
            }

            for (auto& eventprocessor : this->additionalEventProcessors)
            {
                eventprocessor(&sdlevent);
            }
        }

        auto sdl_keyboardstates = SDL_GetKeyboardState(nullptr);
        auto sdl_mousestates = SDL_GetMouseState(nullptr, nullptr);
    }
    void Window::SwapBuffers()
    {
        //SDL_GL_SwapWindow(this->implemented_window);
    }
    bool Window::ShouldClose()
    {
        return this->shouldclose;
    }
	bool Window::isMinimized()
	{
        return SDL_GetWindowFlags(this->implemented_window) & SDL_WINDOW_MINIMIZED;
    }
    Vector2Int Window::Get_dimentions()
    {
        return this->dimentions;
    }
    void Window::RegisterWindowCallback(const std::string key, std::function<void(void*)> func)
    {
        this->window_callbacks.insert_or_assign(key, func);
    }
    void Window::InvokeWindowCallback(const std::string key, void* parameter_object)
    {
        if (this->window_callbacks.find(key) == this->window_callbacks.end()) {
            return;
        }
        else {
            this->window_callbacks[key](parameter_object);
        }
    }
    bool gbe::Window::GetKeyState(unsigned int keyId)
    {
        auto state = this->keystates[keyId];
        return state;
    }
    Vector2Int Window::GetMousePixelPos()
    {
        SDL_GetMouseState(&this->mousePos.x, &this->mousePos.y);
        this->mousePos -= viewport_offset;

        return this->mousePos;
    }
    Vector2 Window::GetMouseDecimalPos()
    {
        SDL_GetMouseState(&this->mousePos.x, &this->mousePos.y);
        this->mousePos -= viewport_offset;

        auto SDL_decimalpos = Vector2(
            (float)this->mousePos.x / (this->dimentions.x * this->viewport_subscale.x),
            (float)this->mousePos.y / (this->dimentions.y * this->viewport_subscale.y)
        );
        auto normalized_decimalpos = Vector2(SDL_decimalpos.x * 2.0f - 1.0f, (1.0f - SDL_decimalpos.y) * 2.0f - 1.0f);

        return normalized_decimalpos;
    }
    void Window::Set_cursorLock(bool locked) {
        SDL_ShowCursor(locked ? SDL_DISABLE : SDL_ENABLE);
        SDL_SetRelativeMouseMode(locked ? SDL_TRUE : SDL_FALSE);
        
        centered_cursor = locked;
    }

    void Window::AddAdditionalEventProcessor(std::function<void(void*)> processor) {
        this->additionalEventProcessors.push_back(processor);
    }
}