#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec3 VertexColor;
layout (location = 1) out vec2 TexCoord;

layout (set = 1, binding = 0) uniform UniformBlock {
	mat4 Transform;
};

void main()
{
	gl_Position = Transform * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	VertexColor = aColor;
	TexCoord = aTexCoord;
}
