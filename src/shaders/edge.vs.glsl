#version 330 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec3 normal;

uniform mat4 mvp;

void main()
{
    vec3 expandedPos = position + normal * 0.015;
    gl_Position = mvp * vec4(expandedPos, 1.0);
}