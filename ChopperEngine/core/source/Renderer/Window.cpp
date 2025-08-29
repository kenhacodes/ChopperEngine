#include "Window.h"
#include <Renderer/Renderer.h>
#include <iostream>
#include <stb/stb_image.h>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

void Window::windowInit(Renderer& renderer)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); 

    monitors_ = glfwGetMonitors(&monitors_count_);
    window_ = glfwCreateWindow(WIDTH, HEIGHT, "Chopper Engine", nullptr, nullptr);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetWindowPosCallback(window_, framebufferResizeCallback);
    
    // load chopper icon
    GLFWimage images[1];
    int width, height, channels;

    unsigned char* pixels = stbi_load("../core/resources/icons/chopper.png", &width, &height, &channels, 4);
    if (pixels)
    {
        images[0].width = width;
        images[0].height = height;
        images[0].pixels = pixels;
        glfwSetWindowIcon(window_, 1, images);
        stbi_image_free(pixels);
    }
    else std::cerr << "Failed to load window icon!" << std::endl;
}


static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto renderer = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    renderer->framebuffer_resized_ = true;
}
