#pragma once
#include "Renderer/Renderer.h"


namespace Chopper
{
    class Engine
    {
    public:
        Engine() = default;
        ~Engine();

        Renderer renderer;
        
        double delta_time = 0.0;
        double last_frame_time = 0.0;

        void init();
        void cleanup();
        GLFWwindow* getGLFWWindow() const;
    };
}
