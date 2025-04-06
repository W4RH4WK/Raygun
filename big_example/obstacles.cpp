#include "obstacles.hpp"
#include "raygun/raygun.hpp"

using namespace raygun;
using namespace raygun::physics;
using namespace physx;

Obstacles::Obstacles(raygun::Entity* parent, int count)
    : m_generator(std::random_device()())
{
    RAYGUN_INFO("Obstacles: Creating {} random obstacles", count);
    
    for (int i = 0; i < count; ++i) {
        createRandomObstacle(parent, i);
    }
}

void Obstacles::createRandomObstacle(raygun::Entity* parent, int index)
{
    // Random distributions
    std::uniform_real_distribution<float> distX(AREA_X_MIN, AREA_X_MAX);
    std::uniform_real_distribution<float> distY(AREA_Y_MIN, AREA_Y_MAX);
    std::uniform_real_distribution<float> distZ(AREA_Z_MIN, AREA_Z_MAX);
    std::uniform_real_distribution<float> distSize(MIN_SIZE, MAX_SIZE);
    // 70% chance of cube, 30% chance of sphere
    std::discrete_distribution<int> distType({70, 30});
    
    const bool isCube = distType(m_generator) == 0;
    const std::string name = isCube ? "Cube" : "Sphere";
    const float size = distSize(m_generator);
    
    RAYGUN_INFO("Obstacles: Creating random {} at index {}", name, index);
    
    // Create entity
    auto entity = std::make_shared<Entity>(name + "_" + std::to_string(index));
    
    // Create and assign random material
    auto material = createRandomMaterial();
    
    // Create cube or sphere
    if (isCube) {
        // Load the cube model
        auto cubeEntity = RG().resourceManager().loadEntity("cube");
        
        if (!cubeEntity->children().empty()) {
            entity->model = std::move(cubeEntity->children().at(0)->model);
            
            // Apply our material
            if (entity->model) {
                // Add our material to the model's materials array
                entity->model->materials.clear();
                entity->model->materials.push_back(material);
            }
            
            // Position and scale the cube
            entity->scale(vec3(size));
            entity->moveTo(vec3(distX(m_generator), distY(m_generator), distZ(m_generator)));
            
            // Add physics as a box
            RG().physicsSystem().attachRigidDynamic(*entity, false, GeometryType::BoundingBox);
        }
    } else {
        // For spheres, also use the ball model
        auto ballModel = RG().resourceManager().loadEntity("ball");
        
        // Extract model from the ball entity's first child
        if (!ballModel->children().empty()) {
            entity->model = std::move(ballModel->children().at(0)->model);
            
            // Set our custom material
            if (entity->model) {
                // Add our material to the model's materials array
                entity->model->materials.clear();
                entity->model->materials.push_back(material);
            }
            
            // Position and scale the sphere
            entity->scale(vec3(size));
            entity->moveTo(vec3(distX(m_generator), distY(m_generator), distZ(m_generator)));
            
            // Add physics as a sphere
            RG().physicsSystem().attachRigidDynamic(*entity, false, GeometryType::Sphere);
        }
    }
    
    // Configure physics properties
    auto* actor = entity->physicsActor.get();
    if (actor && actor->is<PxRigidDynamic>()) {
        auto* rigidBody = static_cast<PxRigidDynamic*>(actor);
        // Set mass proportional to size, with cubes being heavier than spheres
        float mass = isCube ? 20.0f * (size * size * size) : 10.0f * (size * size * size);
        PxRigidBodyExt::updateMassAndInertia(*rigidBody, mass);
    }
    
    // Add to scene and track in our obstacles vector
    parent->addChild(entity);
    m_obstacles.push_back(entity);
}

std::shared_ptr<raygun::Material> Obstacles::createRandomMaterial()
{
    // Random color components
    std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
    std::uniform_real_distribution<float> distRoughness(0.1f, 0.9f);
    
    // Create a new material with random properties
    auto material = std::make_shared<Material>();
    
    // Set random diffuse color - these properties are on the gpu::Material struct
    material->gpuMaterial.diffuse = vec3(distColor(m_generator), distColor(m_generator), distColor(m_generator));
    
    // Set specular (usually white or gray for most materials)
    material->gpuMaterial.specular = vec3(0.2f, 0.2f, 0.2f);
    
    // Set roughness
    material->gpuMaterial.roughness = distRoughness(m_generator);
    
    return material;
}