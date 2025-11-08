#version 460 core
layout (location = 0) in vec3 VertexColor;
layout (location = 1) in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D containerTexture;
layout (set = 2, binding = 1) uniform sampler2D awesomefaceTexture;

void main()
{
    FragColor = mix(texture(containerTexture, TexCoord), texture(awesomefaceTexture, vec2(TexCoord.x, TexCoord.y)), 0.2f);
}
