#include "Core/Core.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "Core/HelloTriangle.h"

int main()
{
    //Chopper::PrintHelloWorld();

    try
    {
        Chopper::HelloTriangleApplication app;
        printf("Running Vulkan app...\n");
#if !defined NDEBUG
        printf("DEBUG MODE.\n");
#endif
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
