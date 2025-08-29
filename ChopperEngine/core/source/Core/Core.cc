#include "Core.h"

#include <iostream>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <cstdlib>

namespace Chopper
{
    Engine::~Engine()
    {
        cleanup();
    }

    void Engine::init()
    {
        renderer.init();
    }

    GLFWwindow* Engine::getGLFWWindow() const
    {
        return renderer.window_.window_;
    }

    void Engine::cleanup()
    {
        renderer.cleanUp();

        printf("\n"
            "-----------------------------------------------------------------\n "
            "       I realized that back then, the reason I wanted\n"
            " to become human, was that I really just wanted to have friends.\n"
            "     Now, I just want to be a monster that can help Luffy.\n"
            "-----------------------------------------------------------------\n\n");
    }
}
