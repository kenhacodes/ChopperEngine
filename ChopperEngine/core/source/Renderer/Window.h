#pragma once

#include "GLFW/glfw3.h"

class Renderer;

// Initial Default Window Size
constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 1080;

class Window
{
public:
    Window() = default;
   

    void windowInit(Renderer& renderer);
    
    GLFWwindow* window_ = nullptr;
    GLFWmonitor** monitors_ = nullptr;
    int monitors_count_ = 0;
    bool framebuffer_resized_ = false;
    //Renderer* renderer_ = nullptr; 

private:
    // TODO Save last used window size and initialize it in that size.
    
};
