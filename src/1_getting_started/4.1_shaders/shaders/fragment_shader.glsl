#version 460
layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform UniformBlock {
    vec4 triangleColor;
};

void main()
{
    FragColor = triangleColor;
}
