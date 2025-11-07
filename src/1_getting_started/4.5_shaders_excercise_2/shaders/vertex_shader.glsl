#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 0) out vec3 vertexColor;

layout (set = 1, binding = 0) uniform UniformBlock
{
	float xOffset;
};

void main()
{
	gl_Position = vec4(aPos.x + xOffset, aPos.y, aPos.z, 1.0);
	vertexColor = aColor;
}
