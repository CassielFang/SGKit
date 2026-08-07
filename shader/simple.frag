#version 330 core

// SGKit Simple Fragment Shader  (light markers, debug objects)
//
// Renders a solid emissive colour.  The Renderer sets u_simpleColor via
// shader.SetVector3("u_simpleColor", ...) before drawing.

uniform vec3 u_simpleColor = vec3(1.0, 1.0, 1.0);

in vec2 texCoord;
in vec3 worldPos;
in vec3 normal;
out vec4 fragColor;

void main()
{
    fragColor = vec4(u_simpleColor, 1.0);
}
