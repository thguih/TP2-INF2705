#version 330 core

in vec3 vColor;
in vec2 vTexCoords;

uniform vec3 uColorMod;
uniform sampler2D diffuseSampler;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(diffuseSampler, vTexCoords);
    FragColor = tex * vec4(vColor * uColorMod, 1.0);
}