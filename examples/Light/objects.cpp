#include "objects.h"
#include <sgkit/sgkit.h>

constexpr float objVertices[] = {
        -0.5f, -0.5f,  0.5f,  0,0,1,  0,0,   0.5f, -0.5f,  0.5f,  0,0,1,  1,0,
         0.5f,  0.5f,  0.5f,  0,0,1,  1,1,  -0.5f,  0.5f,  0.5f,  0,0,1,  0,1,
         0.5f, -0.5f, -0.5f,  0,0,-1, 0,0,  -0.5f, -0.5f, -0.5f,  0,0,-1, 1,0,
        -0.5f,  0.5f, -0.5f,  0,0,-1, 1,1,   0.5f,  0.5f, -0.5f,  0,0,-1, 0,1,
        -0.5f,  0.5f,  0.5f,  0,1,0,  0,0,   0.5f,  0.5f,  0.5f,  0,1,0,  1,0,
         0.5f,  0.5f, -0.5f,  0,1,0,  1,1,  -0.5f,  0.5f, -0.5f,  0,1,0,  0,1,
        -0.5f, -0.5f, -0.5f,  0,-1,0, 0,0,   0.5f, -0.5f, -0.5f,  0,-1,0, 1,0,
         0.5f, -0.5f,  0.5f,  0,-1,0, 1,1,  -0.5f, -0.5f,  0.5f,  0,-1,0, 0,1,
         0.5f, -0.5f,  0.5f,  1,0,0,  0,0,   0.5f, -0.5f, -0.5f,  1,0,0,  1,0,
         0.5f,  0.5f, -0.5f,  1,0,0,  1,1,   0.5f,  0.5f,  0.5f,  1,0,0,  0,1,
        -0.5f, -0.5f, -0.5f, -1,0,0,  0,0,  -0.5f, -0.5f,  0.5f, -1,0,0,  1,0,
        -0.5f,  0.5f,  0.5f, -1,0,0,  1,1,  -0.5f,  0.5f, -0.5f, -1,0,0,  0,1,
};
constexpr uint32_t objIndices[] = {
     0, 1, 2, 2, 3, 0,    4, 5, 6, 6, 7, 4,
     8, 9,10,10,11, 8,   12,13,14,14,15,12,
    16,17,18,18,19,16,   20,21,22,22,23,20,
};

scene::Entity createCube(
    const scene::component::Transform& transform, std::shared_ptr<graphics::Shader> shader,
    std::shared_ptr<graphics::Texture>diff, std::shared_ptr<graphics::Texture>spec)
{
    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(objVertices, sizeof(objVertices));
    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(objIndices, sizeof(objIndices) / sizeof(uint32_t));

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    auto material = std::make_shared<scene::Material>();
    material->shader = shader;
    material->diffuse = diff;
    material->specular = spec;

    auto mesh = std::make_shared<scene::Mesh>();
    mesh->vertexArray = va;
    mesh->material = material;

    auto& sceneManager = scene::Scene::instance();
    auto entity = sceneManager.CreateEntity();
    *sceneManager.AddComponent<scene::component::Transform>(entity) = transform;
    sceneManager.AddComponent<scene::component::MeshRenderer>(entity)->mesh = mesh;

    return entity;
}

scene::Entity createCamera()
{
    auto& sceneManager = scene::Scene::instance();
    auto entity = sceneManager.CreateEntity();
    sceneManager.AddComponent<scene::component::Camera>(entity);
    auto transform = sceneManager.AddComponent<scene::component::Transform>(entity);
    transform->position = { 0.0f, 0.0f, 8.0f };
    return entity;
}

scene::Entity createLight()
{
    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(objVertices, sizeof(objVertices));
    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(objIndices, sizeof(objIndices) / sizeof(uint32_t));

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    auto shader = std::make_shared<graphics::Shader>();
    shader->LoadFromFile("assets/shaders/simple.vert", "assets/shaders/simple.frag");

    auto material = std::make_shared<scene::Material>();
    material->shader = shader;

    auto mesh = std::make_shared<scene::Mesh>();
    mesh->vertexArray = va;
    mesh->material = material;

    auto& sceneManager = scene::Scene::instance();
    auto entity = sceneManager.CreateEntity();
    auto transform = sceneManager.AddComponent<scene::component::Transform>(entity);
    transform->position = { 2.0f, 1.0f, 5.0f };
    //transform->position = { -0.2f, -1.0f, -0.3f };
    sceneManager.AddComponent<scene::component::MeshRenderer>(entity)->mesh = mesh;
    auto light = sceneManager.AddComponent<scene::component::Light>(entity);
    light->type = scene::component::Light::Type::SpotLight;
    light->direction = math::Vector3{ 0.0f, 0.0f, -1.0f };
    light->ambient = math::Vector3{ 0.2f, 0.2f, 0.2f };
    light->diffuse = math::Vector3{ 2.4f, 2.25f, 1.8f };
    light->specular = math::Vector3{ 1.0f, 1.0f, 1.0f };

    return entity;
}
