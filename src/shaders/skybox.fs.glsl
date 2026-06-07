#version 410 core

in vec3 viewDir;
out vec4 FragColor;

uniform sampler2D skyTexture;
uniform float exposure;

const float PI = 3.14159265359;

void main()
{
    vec3 dir = normalize(viewDir);

    // Equirectangular mapping: direction -> UV
    float phi = atan(dir.z, dir.x);       // azimuthal [-PI, PI]
    float theta = asin(clamp(dir.y, -1.0, 1.0)); // elevation [-PI/2, PI/2]

    vec2 uv = vec2(phi / (2.0 * PI) + 0.5, -(theta / PI + 0.5) + 1.0);

    vec3 hdrColor = texture(skyTexture, uv).rgb;

    // Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
