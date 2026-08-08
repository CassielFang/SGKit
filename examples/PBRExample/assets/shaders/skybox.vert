#version 330 core

// SGKit — Skybox vertex shader
// Renders a unit cube centred at the camera.  The view matrix's
// translation column is stripped so the skybox never moves.

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjectionSky;   // proj * view (no translation)
out vec3 v_TexCoord;

void main()
{
    v_TexCoord  = a_Position;
    gl_Position = (u_ViewProjectionSky * vec4(a_Position, 1.0)).xyww;
    // .xyww ensures depth == 1.0 (far plane) after perspective divide
}
