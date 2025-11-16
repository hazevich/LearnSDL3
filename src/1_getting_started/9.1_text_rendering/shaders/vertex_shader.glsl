#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

layout (location = 0) out vec2 TexCoord;

layout (set = 1, binding = 0) uniform MatrixUniform {
	mat4 Model;
	mat4 View;
	mat4 Projection;
};

void main()
{
	gl_Position = Projection * View * vec4(aPos.x, aPos.y, 0.0f, 1.0);
	TexCoord = aTexCoord;
}
