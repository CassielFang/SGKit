#include <sgkit/scripting/Interop.h>

#include <sgkit/scene/Scene.h>
#include <sgkit/scene/Components.h>
#include <sgkit/core/Input.h>
#include <sgkit/core/KeyCodes.h>
#include <sgkit/framework/Timing.h>
#include <sgkit/framework/DebugOut.h>
#include <sgkit/math/Quaternion.h>
#include <sgkit/math/Vector3.h>

namespace {

using sgkit::scene::Entity;
using sgkit::scene::component::Transform;

// Fetch the Transform of an entity id, or nullptr.
Transform* GetTransform(unsigned int entity)
{
    return sgkit::scene::Scene::instance().GetComponent<Transform>(Entity(entity));
}

} // namespace

extern "C" {

// -- Logging -----------------------------------------------------------------

void SGK_Log(int level, const char* message)
{
    if (!message) return;
    switch (level)
    {
        case 1:  SGK_LOG_WARN ("C#", "%s", message); break;
        case 2:  SGK_LOG_ERROR("C#", "%s", message); break;
        case 3:  SGK_LOG_FATAL("C#", "%s", message); break;
        default: SGK_LOG_INFO ("C#", "%s", message); break;
    }
}

// -- Entity ------------------------------------------------------------------

int SGK_Entity_IsAlive(unsigned int entity)
{
    return sgkit::scene::Scene::instance().IsAlive(Entity(entity)) ? 1 : 0;
}

// -- Transform ---------------------------------------------------------------

int SGK_Transform_Has(unsigned int entity)
{
    return GetTransform(entity) != nullptr ? 1 : 0;
}

void SGK_Transform_GetPosition(unsigned int entity, sgkit::scripting::Vec3* out)
{
    if (!out) return;
    if (Transform* t = GetTransform(entity))
    { out->x = t->position.x; out->y = t->position.y; out->z = t->position.z; }
    else
    { out->x = out->y = out->z = 0.0f; }
}

void SGK_Transform_SetPosition(unsigned int entity, const sgkit::scripting::Vec3* value)
{
    if (!value) return;
    if (Transform* t = GetTransform(entity))
        t->position = { value->x, value->y, value->z };
}

void SGK_Transform_GetEuler(unsigned int entity, sgkit::scripting::Vec3* out)
{
    if (!out) return;
    if (Transform* t = GetTransform(entity))
    {
        sgkit::math::Vector3 e = t->rotation.ToEulerAngles();
        out->x = e.x; out->y = e.y; out->z = e.z;
    }
    else { out->x = out->y = out->z = 0.0f; }
}

void SGK_Transform_SetEuler(unsigned int entity, const sgkit::scripting::Vec3* value)
{
    if (!value) return;
    if (Transform* t = GetTransform(entity))
        t->rotation = sgkit::math::Quaternion::FromEulerAngles(value->x, value->y, value->z);
}

void SGK_Transform_GetScale(unsigned int entity, sgkit::scripting::Vec3* out)
{
    if (!out) return;
    if (Transform* t = GetTransform(entity))
    { out->x = t->scale.x; out->y = t->scale.y; out->z = t->scale.z; }
    else { out->x = out->y = out->z = 1.0f; }
}

void SGK_Transform_SetScale(unsigned int entity, const sgkit::scripting::Vec3* value)
{
    if (!value) return;
    if (Transform* t = GetTransform(entity))
        t->scale = { value->x, value->y, value->z };
}

// -- Input -------------------------------------------------------------------

int SGK_Input_IsKeyDown(int key)
{
    return sgkit::core::Input::instance()
        .IsKeyDown(static_cast<sgkit::core::KeyCode>(key)) ? 1 : 0;
}

int SGK_Input_IsKeyPressed(int key)
{
    return sgkit::core::Input::instance()
        .IsKeyPressed(static_cast<sgkit::core::KeyCode>(key)) ? 1 : 0;
}

// -- Clock -------------------------------------------------------------------

float SGK_Clock_DeltaTime(void)
{
    return sgkit::framework::Clock::GetFrameDeltaSeconds();
}

} // extern "C"
