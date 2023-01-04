#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 aNormal;

out vec4 ourColor;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	gl_Position = projection * view * model * vec4(a_Position, 1.0f);
	ourColor = vec4(aNormal, 1.0f);
}