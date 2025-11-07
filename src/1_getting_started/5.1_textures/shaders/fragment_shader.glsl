#version 460 core
layout (location = 0) in vec3 VertexColor;
layout (location = 1) in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D containerTexture;

void main()
{
    FragColor = texture(containerTexture, TexCoord) * vec4(VertexColor, 1.0f);
}
