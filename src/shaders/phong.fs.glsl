#version 330 core

#define MAX_SPOT_LIGHTS 12

in ATTRIBS_VS_OUT
{
    vec2 texCoords;
    vec3 normal;
    vec3 color;
} attribsIn;

in LIGHTS_VS_OUT
{
    vec3 obsPos;
    vec3 dirLightDir;
    
    vec3 spotLightsDir[MAX_SPOT_LIGHTS];
    vec3 spotLightsSpotDir[MAX_SPOT_LIGHTS];
} lightsIn;

struct Material
{
    vec3 emission;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirectionalLight
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    vec3 direction;
};

struct SpotLight
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    vec3 position;
    vec3 direction;
    float exponent;
    float openingAngle;
};

uniform int nSpotLights;
uniform vec3 globalAmbient;

layout (std140) uniform MaterialBlock
{
    Material mat;
};

layout (std140) uniform LightingBlock
{
    DirectionalLight dirLight;
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

uniform sampler2D diffuseSampler;

out vec4 FragColor;

float computeSpot(in float openingAngle, in float exponent, in vec3 spotDir, in vec3 lightDir)
{
    float cosGamma = dot(normalize(lightDir), normalize(spotDir));
    float cosDelta = cos(radians(openingAngle));

    if (cosGamma > cosDelta)
        return pow(cosGamma, exponent);

    return 0.0;
}

void main()
{
    vec4 texColor = texture(diffuseSampler, attribsIn.texCoords);

    vec3 O = normalize(lightsIn.obsPos);

    // Normalisation après rasterisation
    vec3 N = normalize(attribsIn.normal);

    // Utile pour les objets dessinés double-face comme l’arbre
    if (!gl_FrontFacing)
        N = -N;

    vec3 kd = texColor.rgb * mat.diffuse;
    vec3 color = mat.emission;

    // Ambiant global + lumière directionnelle ambiante
    color += kd * globalAmbient * mat.ambient;
    color += kd * dirLight.ambient * mat.ambient;

    // Lumière directionnelle
    {
        vec3 L = normalize(-lightsIn.dirLightDir);

        float diff = max(dot(N, L), 0.0);
        vec3 diffuseTerm = dirLight.diffuse * kd * diff;

        vec3 specularTerm = vec3(0.0);
        if (diff > 0.0)
        {
            vec3 R = reflect(-L, N);
            float spec = pow(max(dot(R, O), 0.0), mat.shininess);
            specularTerm = dirLight.specular * mat.specular * spec;
        }

        color += diffuseTerm + specularTerm;
    }

    // Spotlights
    for (int i = 0; i < nSpotLights; i++)
    {
        vec3 Lvec = lightsIn.spotLightsDir[i];
        float dist = length(Lvec);
        vec3 L = normalize(Lvec);

        float spotFactor = computeSpot(
            spotLights[i].openingAngle,
            spotLights[i].exponent,
            lightsIn.spotLightsSpotDir[i],
            L
        );

        if (spotFactor > 0.0)
        {
            // Atténuation personnalisée du TP
            float attenuation = 1.0 - smoothstep(7.0, 10.0, dist);

            float diff = max(dot(N, L), 0.0);
            vec3 diffuseTerm = spotLights[i].diffuse * kd * diff;

            vec3 specularTerm = vec3(0.0);
            if (diff > 0.0)
            {
                vec3 R = reflect(-L, N);
                float spec = pow(max(dot(R, O), 0.0), mat.shininess);
                specularTerm = spotLights[i].specular * mat.specular * spec;
            }

            vec3 ambientTerm = spotLights[i].ambient * kd * mat.ambient;

            color += (ambientTerm + diffuseTerm + specularTerm) * spotFactor * attenuation;
        }
    }

    FragColor = vec4(color, texColor.a);
}