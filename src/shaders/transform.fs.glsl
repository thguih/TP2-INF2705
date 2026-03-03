#version 330 core

// TODO: La couleur des fragments est donnée à partir de la couleur
//       des vertices passée en entrée.
//       De plus, une variable uniform permet de multiplier la couleur
//       par une autre pour coloriser les fragments.

in vec3 vColor; 
out vec4 FragColor;

uniform vec3 uColorMod;

void main()
{
    FragColor = vec4(vColor * uColorMod, 1.0);
}
