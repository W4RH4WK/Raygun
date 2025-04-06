#include "raygun/raygun.hpp"
#include "raygun/logging.hpp"

#include "example_scene.hpp"

using namespace raygun;

int main()
{
    try {
        RAYGUN_INFO("Starting Raygun example application");
        
        RAYGUN_INFO("Initializing Raygun engine");
        Raygun rg;
        
        RAYGUN_INFO("Loading example scene");
        rg.loadScene(std::make_unique<ExampleScene>());
        
        RAYGUN_INFO("Entering main loop");
        rg.loop();
        
        RAYGUN_INFO("Application shutting down normally");
    } catch (const std::exception& e) {
        RAYGUN_ERROR("Exception: {}", e.what());
        return 1;
    } catch (...) {
        RAYGUN_ERROR("Unknown exception occurred");
        return 1;
    }
    
    return 0;
}
