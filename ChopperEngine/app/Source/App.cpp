#include "Core/Core.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "Renderer/ChopperImGui.h"

int main()
{
#if !defined NDEBUG
    printf("DEBUG MODE.\n");
#endif
    {
        Chopper::Engine engine;
        engine.init();

        while (!glfwWindowShouldClose(engine.getGLFWWindow()))
        {
            double current_time = glfwGetTime(); // time in seconds since glfwInit
            engine.delta_time = current_time - engine.last_frame_time;
            engine.last_frame_time = current_time;

            ChopperImGui::imgui_initFrame();

            if (engine.renderer.show_demo_window)
                ImGui::ShowDemoWindow(&engine.renderer.show_demo_window);

            engine.renderer.mainLoop(engine.delta_time);
        }
    }
    return 0;
}
