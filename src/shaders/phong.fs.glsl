#version 330 core

#define MAX_SPOT_LIGHTS 12

in ATTRIBS_VS_OUT
{
    vec2 texCoords;
    vec3 normal;
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

uniform bool useOrbGradient;
uniform vec3 orbInnerColor;
uniform vec3 orbOuterColor;

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
    vec3 O = normalize(lightsIn.obsPos);
    vec3 N = normalize(attribsIn.normal);

    if (!gl_FrontFacing)
        N = -N;

    if (useOrbGradient)
    {
        float fresnel = pow(1.0 - max(dot(N, O), 0.0), 2.2);
        float vertical = clamp(attribsIn.texCoords.y, 0.0, 1.0);
        float horizontal = clamp(attribsIn.texCoords.x, 0.0, 1.0);

        vec3 color = mix(orbInnerColor, orbOuterColor, fresnel);
        color = mix(color, orbOuterColor * 1.08, 0.22 * vertical);
        color = mix(color, orbInnerColor * 1.05, 0.12 * (1.0 - horizontal));

        FragColor = vec4(color, 1.0);
        return;
    }

    vec4 texSample = texture(diffuseSampler, attribsIn.texCoords);
    vec3 texColor = texSample.rgb;

    vec3 kd = texColor * mat.diffuse;
    vec3 color = mat.emission;

    color += kd * 0.28;
    color += kd * globalAmbient * mat.ambient;
    color += kd * dirLight.ambient * mat.ambient;

    {
        vec3 L = normalize(-lightsIn.dirLightDir);

        // Half-Lambert (Wrapped lighting) for much smoother light distribution
        float NdotL = dot(N, L);
        float diff = max(NdotL * 0.5 + 0.5, 0.0);
        
        // Square it for a more natural falloff, or just use the Half-Lambert natively
        diff = diff * diff; 

        vec3 diffuseTerm = dirLight.diffuse * kd * diff;

        vec3 specularTerm = vec3(0.0);
        if (NdotL > 0.0) // Specular only where light directly hits
        {
            vec3 R = reflect(-L, N);
            float spec = pow(max(dot(R, O), 0.0), mat.shininess);
            specularTerm = dirLight.specular * mat.specular * spec;
        }

        color += diffuseTerm + specularTerm;
    }

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
            float attenuation = 1.0 / (1.0 + 0.08 * dist + 0.03 * dist * dist);

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

    color = clamp(color, 0.0, 1.0);
    FragColor = vec4(color, texSample.a);
}