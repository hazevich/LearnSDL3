#version 460
layout (location = 0) in vec3 vertexColor;
layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor.xyz, 1.0f);
}
