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


float computeSpot(in float openingAngle, in float exponent, in vec3 spotDir, in vec3 lightDir, in vec3 normal)
{
    float spotFactor = 0.0;
    
    float cosGamma = dot(normalize(lightDir), normalize(spotDir));
    float cosDelta = cos(radians(openingAngle));

    if (cosGamma > cosDelta)
        spotFactor = pow(cosGamma, exponent);
    
    return spotFactor;
}

void main()
{
    vec3 N = normalize(attribsIn.normal);
    vec3 O = normalize(lightsIn.obsPos);

    vec4 texColor = texture(diffuseSampler, attribsIn.texCoords);
    vec3 diffuseTex = texColor.rgb * mat.diffuse;

    vec3 color = mat.emission + globalAmbient * mat.ambient;

    {
        vec3 L = normalize(-lightsIn.dirLightDir);
        float diff = max(dot(N, L), 0.0);

        // Cel shading: quantize diffuse and specular
        const float LEVELS = 4.0;
        diff = floor(diff * LEVELS) / LEVELS;

        vec3 R = reflect(-L, N);
        float spec = 0.0;
        if (diff > 0.0)
        {
            spec = pow(max(dot(R, O), 0.0), mat.shininess);
            spec = floor(spec * LEVELS) / LEVELS;
        }

        color += dirLight.ambient * mat.ambient;
        color += dirLight.diffuse * diffuseTex * diff;
        color += dirLight.specular * mat.specular * spec;
    }

    for (int i = 0; i < nSpotLights; i++)
    {
        vec3 L = lightsIn.spotLightsDir[i];
        float dist = length(L);
        L = normalize(L);

        float spotFactor = computeSpot(
            spotLights[i].openingAngle,
            spotLights[i].exponent,
            lightsIn.spotLightsSpotDir[i],
            L,
            N
        );

        if (spotFactor > 0.0)
        {
            float attenuation = 1.0 - smoothstep(7.0, 10.0, dist);

            float diff = max(dot(N, L), 0.0);
            vec3 R = reflect(-L, N);
            float spec = 0.0;
            if (diff > 0.0)
                spec = pow(max(dot(R, O), 0.0), mat.shininess);

            color += spotLights[i].ambient * mat.ambient;
            color += spotLights[i].diffuse * diffuseTex * diff * spotFactor * attenuation;
            color += spotLights[i].specular * mat.specular * spec * spotFactor * attenuation;
        }
    }

    FragColor = vec4(color, texColor.a);
}

