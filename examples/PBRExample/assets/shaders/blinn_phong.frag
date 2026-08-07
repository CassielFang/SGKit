#version 330 core

// SGKit Blinn-Phong Fragment Shader
//
// Three light types (directional / point / spot), Blinn-Phong specular (halfway vector
// instead of reflect), soft spot-light edges.  Fully compatible with SGKit Renderer's
// SetFrameUniforms() naming.

// -- Structs (must stay in sync with the vertex shader and Renderer.cpp) ----------

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};

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

// -- Uniforms ----------------------------------------------------------------

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;
uniform vec3 u_cameraPos;
uniform Material u_Material;

uniform DirectionalLight u_DirectionalLight;
uniform PointLight       u_PointLights[4];
uniform SpotLight        u_SpotLights[4];
uniform int u_dLightCount, u_pLightCount, u_sLightCount;

// -- Varyings ----------------------------------------------------------------

in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;
out vec4 fragColor;

// -- Forward declarations ----------------------------------------------------

vec3 CalcDirectionalLight(DirectionalLight light, vec3 N, vec3 V);
vec3 CalcPointLight(PointLight light, vec3 N, vec3 P, vec3 V);
vec3 CalcSpotLight(SpotLight light, vec3 N, vec3 P, vec3 V);

// ============================================================================
// main()
// ============================================================================

void main()
{
    vec3 N = normalize(normal);
    vec3 V = normalize(u_cameraPos - worldPos);

    vec3 color = vec3(0.0);

    if (u_dLightCount > 0)
        color += CalcDirectionalLight(u_DirectionalLight, N, V);

    for (int i = 0; i < u_pLightCount && i < 4; ++i)
        color += CalcPointLight(u_PointLights[i], N, worldPos, V);

    for (int i = 0; i < u_sLightCount && i < 4; ++i)
        color += CalcSpotLight(u_SpotLights[i], N, worldPos, V);

    fragColor = vec4(color, 1.0);
}

// ============================================================================
// Blinn-Phong light helpers
//
// Key difference from Phong:  specular uses dot(N, H)^shininess  (halfway vector)
// instead of  dot(V, reflect(L, N))^shininess  (reflection vector).
// ============================================================================

vec3 CalcDirectionalLight(DirectionalLight light, vec3 N, vec3 V)
{
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(L + V);                              // Blinn halfway vector

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo    = texture(u_Material.diffuse,  texCoord).rgb;
    vec3 specColor = texture(u_Material.specular, texCoord).rgb;
    float specPow  = pow(NdotH, u_Material.shininess);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * albedo  * NdotL;
    vec3 specular = light.specular * specColor * specPow;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 N, vec3 P, vec3 V)
{
    vec3 L = normalize(light.position - P);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo    = texture(u_Material.diffuse,  texCoord).rgb;
    vec3 specColor = texture(u_Material.specular, texCoord).rgb;
    float specPow  = pow(NdotH, u_Material.shininess);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * albedo  * NdotL;
    vec3 specular = light.specular * specColor * specPow;

    float dist  = length(light.position - P);
    float atten = 1.0 / (light.constant + light.linear * dist
                                         + light.quadratic * dist * dist);

    return (ambient + diffuse + specular) * atten;
}

vec3 CalcSpotLight(SpotLight light, vec3 N, vec3 P, vec3 V)
{
    vec3 L = normalize(light.position - P);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo    = texture(u_Material.diffuse,  texCoord).rgb;
    vec3 specColor = texture(u_Material.specular, texCoord).rgb;
    float specPow  = pow(NdotH, u_Material.shininess);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * albedo  * NdotL;
    vec3 specular = light.specular * specColor * specPow;

    // Soft edge: smooth transition between inner & outer cut-off cones
    float theta     = dot(L, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float dist  = length(light.position - P);
    float atten = 1.0 / (light.constant + light.linear * dist
                                         + light.quadratic * dist * dist);

    return (ambient + diffuse + specular) * atten * intensity;
}
