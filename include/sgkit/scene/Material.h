#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Texture.h>
#include <sgkit/math/Vector3.h>

#include <memory>

namespace sgkit {
namespace scene {

enum class BlendMode
{
    Opaque,
    AlphaBlend,
    Additive,
};

enum class CullMode
{
    Back,
    Front,
    None,
};

enum class DepthMode
{
    ReadWrite,
    ReadOnly,
    None,
};

enum class LightingModel
{
    BlinnPhong,   // Phong/Blinn-Phong: diffuse + specular + shininess
    PBR,          // Cook-Torrance metallic-roughness: albedo + metallic + roughness
};

class Material
{
public:
    // -- Shared ---------------------------------------------------------------
    std::shared_ptr<graphics::Shader>  shader;
    LightingModel lightingModel = LightingModel::BlinnPhong;

    BlendMode blendMode = BlendMode::Opaque;
    CullMode  cullMode  = CullMode::Back;
    DepthMode depthMode = DepthMode::ReadWrite;
    int renderQueue = 0;

    // -- Blinn-Phong  (used when lightingModel == BlinnPhong) -----------------
    std::shared_ptr<graphics::Texture> diffuse;
    std::shared_ptr<graphics::Texture> specular;
    float shininess = 32.0f;

    // -- PBR  (used when lightingModel == PBR) --------------------------------
    // Slot convention:  0=albedo  1=metallic  2=roughness  3=normal  4=ao  5=emissive
    std::shared_ptr<graphics::Texture> albedo;
    std::shared_ptr<graphics::Texture> metallic;
    std::shared_ptr<graphics::Texture> roughness;
    std::shared_ptr<graphics::Texture> normalMap;
    std::shared_ptr<graphics::Texture> ao;
    std::shared_ptr<graphics::Texture> emissive;   // self-illumination map (slot 5)

    float metallicFactor  = 1.0f;   // multiplied by metallic texture value
    float roughnessFactor = 1.0f;   // multiplied by roughness texture value
    math::Vector3 emissiveFactor;  // RGB self-illumination (added after lighting), default (0,0,0)
    float alphaFactor = 1.0f;      // baseColorFactor.a, passed to shader for fragColor alpha

    // glTF combined metallicRoughnessTexture: metallic=B, roughness=G
    // (false = separate textures, both in R channel)
    bool pbrCombinedMetallicRoughness = false;
};

}
}
