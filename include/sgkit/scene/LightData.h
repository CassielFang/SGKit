#pragma once

// std140-compatible frame-level uniform data.
// Mirrors the GLSL block  layout(std140, binding=0) uniform FrameBlock { ... };
// Uploaded once per frame via UniformBuffer, shared by all shaders.

namespace sgkit {
namespace scene {

struct alignas(16) FrameUniforms
{
    // -- Camera (offsets 0, 16)
    float viewProjection[16];   // mat4, 64 bytes
    float cameraPos[4];         // vec4  (vec3 padded), 16 bytes
    // offset = 80

    // -- Directional light (each vec3 padded to vec4 = 16 bytes, total 64)
    float dirDirection[4];
    float dirAmbient[4];
    float dirDiffuse[4];
    float dirSpecular[4];
    // offset = 144

    // -- Point light array  (4 * 5 * vec4 = 4 * 80 = 320)
    struct alignas(16) PointData {
        float position[4];
        float ambient[4];
        float diffuse[4];
        float specular[4];
        float attenuation[4];    // constant, linear, quadratic, pad
    };
    PointData pointLights[4];
    // offset = 464

    // -- Spot light array  (4 * 7 * vec4 = 4 * 112 = 448)
    struct alignas(16) SpotData {
        float position[4];
        float direction[4];
        float ambient[4];
        float diffuse[4];
        float specular[4];
        float attenCut[4];       // constant, linear, quadratic, cutOff
        float outerCutPad[4];    // outerCutOff, pad, pad, pad
    };
    SpotData spotLights[4];
    // offset = 912

    // -- Counts + shadow flag (int = 4 bytes, aligned to vec4 -> 16)
    int lightCounts[4];   // dCount, pCount, sCount, shadowsEnabled
    // offset = 928

    // -- Shadow
    float lightSpaceMatrix[16];  // mat4, 64 bytes
    // offset = 992

};
static_assert(alignof(FrameUniforms) == 16, "FrameUniforms must be 16-byte aligned");

}
}
