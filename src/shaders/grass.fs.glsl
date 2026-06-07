#version 400 core

in ATTRIBS_GS_OUT
{
    float height;
    float shimmer;
    vec3 worldPos;
} attribsIn;

out vec4 FragColor;

uniform float time;
uniform vec3 sunLightDir; // Towards the sun
uniform vec3 cameraPos;

void main()
{
    vec3 baseColor = vec3(0.05, 0.05, 0.35);
    vec3 midColor = vec3(0.2, 0.4, 0.9);
    vec3 tipColor = vec3(0.7, 0.9, 1.0);
    
    vec3 color; 

    if (attribsIn.height < 0.5)
        color = mix(baseColor, midColor, attribsIn.height * 2.0);
    else
        color = mix(midColor, tipColor, (attribsIn.height - 0.5) * 2.0);

    float pulse = 0.5 + 0.5 * sin(time * 2.0 + attribsIn.shimmer * 6.28);
    float glow = pow(attribsIn.height, 3.0) * pulse * 0.5;
    color += vec3(glow * 0.3, glow * 0.5, glow);

    // Translucency (Subsurface Scattering approximation)
    vec3 viewDir = normalize(cameraPos - attribsIn.worldPos);
    // When looking at the sun through the grass, dot(viewDir, sunLightDir) approaches 1.0
    float translucency = pow(max(0.0, dot(viewDir, normalize(sunLightDir))), 4.0);
    
    // Add warm sunlight color passing through the grass blades
    vec3 sunTranslucencyColor = vec3(0.8, 0.9, 0.4) * translucency;
    color += sunTranslucencyColor * attribsIn.height; // Max effect at the tips

    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
