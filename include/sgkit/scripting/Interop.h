#pragma once

// -----------------------------------------------------------------------------
// SGKit scripting interop surface  --  SOURCE OF TRUTH for the binding generator
// -----------------------------------------------------------------------------
// This header is the single place that defines the flat C ABI the C# side calls
// into. The tools/ generator parses it to emit:
//   * NativeApi.gen.h              (the function-pointer table + FillNativeApi)
//   * SGKit.Managed/Generated/*.cs (the matching struct + typed wrappers)
//
// Rules that keep it machine-parseable AND ABI-safe - follow them when adding
// functions:
//   * Every exported function is declared inside the extern "C" block below,
//     one per line, prefixed SGK_, returning/taking only blittable types
//     (int, unsigned int, float, pointers to the POD structs here, const char*).
//   * Structs shared with C# are plain PODs of floats (blittable, sequential).
//   * No overloads, no default args, no templates.
// -----------------------------------------------------------------------------

namespace sgkit {
namespace scripting {

// Blittable 3-vector. Layout MUST match managed SGKit.Vec3 (3 sequential floats).
struct Vec3
{
    float x;
    float y;
    float z;
};

extern "C" {

// -- Logging -----------------------------------------------------------------
// level: 0=Info 1=Warn 2=Error 3=Fatal. message is UTF-8, null-terminated.
void  SGK_Log(int level, const char* message);

// -- Entity ------------------------------------------------------------------
int   SGK_Entity_IsAlive(unsigned int entity);

// -- Transform ---------------------------------------------------------------
int   SGK_Transform_Has(unsigned int entity);
void  SGK_Transform_GetPosition(unsigned int entity, Vec3* out);
void  SGK_Transform_SetPosition(unsigned int entity, const Vec3* value);
void  SGK_Transform_GetEuler(unsigned int entity, Vec3* out);       // radians (pitch,yaw,roll)
void  SGK_Transform_SetEuler(unsigned int entity, const Vec3* value);
void  SGK_Transform_GetScale(unsigned int entity, Vec3* out);
void  SGK_Transform_SetScale(unsigned int entity, const Vec3* value);

// -- Input -------------------------------------------------------------------
// key is the integer value of sgkit::core::KeyCode (mirrored by managed SGKit.Key).
int   SGK_Input_IsKeyDown(int key);
int   SGK_Input_IsKeyPressed(int key);

// -- Clock -------------------------------------------------------------------
float SGK_Clock_DeltaTime(void);

} // extern "C"

}
}
