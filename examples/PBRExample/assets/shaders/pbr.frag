#version 330 core

// SGKit PBR Fragment Shader  (Cook-Torrance metallic-roughness + IBL)
//   LearnOpenGL §6  —  https://learnopengl.com/PBR/
//   glTF 2.0          —  metallic-roughness workflow

// Structs

struct DirectionalLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

// Uniforms

// -- Frame
uniform mat4 u_Model;
uniform mat4 u_ViewProjection;
uniform vec3 u_cameraPos;

// -- PBR Material  (slots: 0=albedo  1=metallic  2=roughness  3=normal  4=ao  5=emissive)
uniform sampler2D u_PBR_albedo;
uniform sampler2D u_PBR_metallic;
uniform sampler2D u_PBR_roughness;
uniform sampler2D u_PBR_normal;
uniform sampler2D u_PBR_ao;
uniform sampler2D u_PBR_emissive;
uniform vec3      u_PBR_emissiveFactor = vec3(0.0);

// Material factors (multiply texture values; defaults = 1.0)
uniform float u_PBR_metallicFactor  = 1.0;
uniform float u_PBR_roughnessFactor = 1.0;
uniform float u_PBR_alphaFactor     = 1.0;

// glTF combined metallicRoughnessTexture: when true, metallic=B, roughness=G
uniform int  u_PBR_combinedMR = 0;

// -- Lights (set by Renderer::SetFrameUniforms, compatible with SGKit)
uniform DirectionalLight u_DirectionalLight;
uniform PointLight       u_PointLights[4];
uniform SpotLight        u_SpotLights[4];
uniform int u_dLightCount, u_pLightCount, u_sLightCount;

// -- Shadow map (directional light)
uniform sampler2D u_ShadowMap;
uniform mat4      u_LightSpaceMatrix;
uniform bool      u_ShadowsEnabled = false;

// -- IBL  (optional - set by user; 0 = disabled -> falls back to constant ambient)
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D   u_BRDFLUT;
uniform bool        u_IBLEnabled = false;

// Varyings

in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;
in vec4 fragPosLightSpace;
out vec4 fragColor;

// Constants

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0; // must match prefilter mip count

// Normal mapping - derivative-based TBN (no tangent attribute required)

vec3 GetWorldNormal()
{
    vec3 texN = texture(u_PBR_normal, texCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(texCoord);
    vec2 st2 = dFdy(texCoord);

    vec3 N = normalize(normal);
    vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    vec3 B = normalize(cross(N, T));   // NOTE: re-orthogonalised, not -cross(N,T)
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * texN);
}

// PBR BRDF functions

// GGX / Trowbridge-Reitz normal distribution
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float Nh = max(dot(N, H), 0.0);
    float Nh2 = Nh * Nh;

    float nom   = a2;
    float denom = Nh2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // direct-lighting remap  (IBL uses k = a²/2)

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    return ggx1 * ggx2;
}

// Schlick Fresnel  (F0 = reflectance at normal incidence)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Schlick Fresnel with roughness term  (for IBL specular)
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Per-light PBR radiance  (used by every light type)

vec3 CalcPBRRadiance(
    vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Cook-Torrance BRDF
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator   = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic); // metals have zero diffuse

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Light helpers (SGKit-compatible signatures)

vec3 RadianceDir(
    DirectionalLight light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 L = normalize(-light.direction);
    return CalcPBRRadiance(L, light.diffuse, N, V, albedo, metallic, roughness, F0);
}

vec3 RadiancePoint(
    PointLight light, vec3 N, vec3 V, vec3 P, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 L = normalize(light.position - P);
    float dist  = length(light.position - P);
    float atten = 1.0 / (light.constant + light.linear * dist
                                         + light.quadratic * dist * dist);
    return CalcPBRRadiance(L, light.diffuse * atten, N, V, albedo, metallic, roughness, F0);
}

vec3 RadianceSpot(
    SpotLight light, vec3 N, vec3 V, vec3 P, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 L = normalize(light.position - P);
    float theta     = dot(L, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float dist  = length(light.position - P);
    float atten = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    return CalcPBRRadiance(L, light.diffuse * atten * intensity, N, V, albedo, metallic, roughness, F0);
}

// -- Shadow

float ShadowCalculationDir(vec4 fragPosLS, vec3 normal, vec3 lightDir)
{
    if (!u_ShadowsEnabled) return 0.0;
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float current = proj.z;
    float bias    = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow  = 0.0;

    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closest = texture(u_ShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += current - bias > closest ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main()
{
    // Material properties
    vec3  albedo    = pow(texture(u_PBR_albedo, texCoord).rgb, vec3(2.2));
    vec4  mrSample  = texture(u_PBR_metallic, texCoord); // may be same as roughness
    float metallic  = (u_PBR_combinedMR != 0 ? mrSample.b : mrSample.r) * u_PBR_metallicFactor;
    float roughness =
        (u_PBR_combinedMR != 0 ? mrSample.g : texture(u_PBR_roughness, texCoord).r) * u_PBR_roughnessFactor;
    float ao        = texture(u_PBR_ao, texCoord).r;

    vec3 N = GetWorldNormal();
    vec3 V = normalize(u_cameraPos - worldPos);

    // F0: 0.04 for dielectrics, albedo for metals (linear blend by metallic)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // -- Direct lighting
    vec3 Lo = vec3(0.0);

    if (u_dLightCount > 0)
    {
        float shadow = ShadowCalculationDir(fragPosLightSpace, N, normalize(-u_DirectionalLight.direction));
        Lo += (1.0 - shadow) * RadianceDir(u_DirectionalLight, N, V, albedo, metallic, roughness, F0);
    }

    for (int i = 0; i < u_pLightCount && i < 4; ++i)
        Lo += RadiancePoint(u_PointLights[i], N, V, worldPos, albedo, metallic, roughness, F0);

    for (int i = 0; i < u_sLightCount && i < 4; ++i)
        Lo += RadianceSpot(u_SpotLights[i], N, V, worldPos, albedo, metallic, roughness, F0);

    // -- Ambient (IBL or constant fallback)
    vec3 ambient;

    if (u_IBLEnabled)
    {
        // IBL diffuse  - irradiance map lookup by surface normal
        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo;

        // IBL specular - split-sum approximation
        vec3 R = reflect(-V, N);
        vec3 prefiltered = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;

        vec3 F  = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        vec3 specularIBL = prefiltered * (F * brdf.x + brdf.y);

        ambient = (kD * diffuseIBL + specularIBL) * ao;
    }
    else
    {
        // Fallback: simple constant ambient (temporary until IBL is set up)
        ambient = vec3(0.03) * albedo * ao;
    }

    // -- Emissive
    vec3 emissiveCol = pow(texture(u_PBR_emissive, texCoord).rgb, vec3(2.2)) * u_PBR_emissiveFactor;

    vec3 color = ambient + Lo + emissiveCol;

    // -- HDR -> LDR
    color = color / (color + vec3(1.0));             // Reinhard tone-mapping
    color = pow(color, vec3(1.0 / 2.2));             // gamma correction

    float alpha = texture(u_PBR_albedo, texCoord).a * u_PBR_alphaFactor;
    fragColor = vec4(color, alpha);
}
