#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;
out vec2 vTexCoords;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);

    vColor = aColor;

    // Generate texture coordinates from world position
    vTexCoords = aPos.xz;
}