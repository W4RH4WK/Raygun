#include "raygun/raygun.hpp"

#include "example_scene.hpp"

using namespace raygun;

int main()
{
    Raygun rg;
    rg.loadScene(std::make_unique<ExampleScene>());
    rg.loop();
}
