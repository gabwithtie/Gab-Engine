/*
Primary file to change if changing window backends. This is the primary interface that connects the engine to the window.

Current backend used: SDL2
*/

#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <queue>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "Engine/Input/KeyDefines.h"

#include "../Math/gbe_math.h"
#include "Window/WindowEvents.h"

namespace gbe {

    class Window {
    private:
        SDL_Window* implemented_window = nullptr;
        Vector2Int mousePos;

        Vector2Int dimentions;
        Vector2 viewport_subscale;
        Vector2Int viewport_offset;

        std::unordered_map<std::string, std::function<void(void*)>> window_callbacks;
        std::unordered_map<unsigned int, bool> keystates;
        
        bool shouldclose = false;

        std::queue<gbe::window::WindowEventType> event_queue;

        bool centered_cursor = false;

        std::vector<std::function<void(void*)>> additionalEventProcessors;
    public:
        ~Window();
        Window();
        Window(Vector2Int dimentions);
        void* Get_implemented_window();

        bool PollEvents(gbe::window::WindowEventType& event_type);

        void UpdateState();
        void SwapBuffers();

        bool ShouldClose();
        bool isMinimized();

        Vector2Int Get_dimentions();

        void RegisterWindowCallback(const std::string, std::function<void(void*)>);
        void InvokeWindowCallback(const std::string, void*);

        bool GetKeyState(unsigned int keyId);
        Vector2Int GetMousePixelPos();
        Vector2 GetMouseDecimalPos();
        void Set_cursorLock(bool);

        void AddAdditionalEventProcessor(std::function<void(void*)> processor);

        inline void Set_viewport(Vector2 viewport_subscale, Vector2Int viewport_offset) {
            this->viewport_subscale = viewport_subscale;
            this->viewport_offset = viewport_offset;
        }
    };
}