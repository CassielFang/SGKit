#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Texture.h>
#include <sgkit/scene/Entity.h>
#include <sgkit/scene/Components.h>

using namespace sgkit;

scene::Entity createCube(
    const scene::component::Transform& transform, std::shared_ptr<graphics::Shader> shader,
    std::shared_ptr<graphics::Texture>diff, std::shared_ptr<graphics::Texture>spec);
scene::Entity createCamera();
scene::Entity createLight();
