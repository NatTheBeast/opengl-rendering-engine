#version 400 core

uniform float time;

in vec2 groundPos;
out vec4 FragColor;

void main()
{
    vec3 baseColor = vec3(0.06, 0.04, 0.10);
    vec3 glowColor = vec3(0.55, 0.20, 0.75);

    float pulse = 0.5 + 0.5 * sin(time * 2.0);

    float dist = length(groundPos);
    float glowMask = 1.0 - smoothstep(2.0, 10.0, dist);

    float glow = glowMask * (0.35 + 0.65 * pulse);

    vec3 color = baseColor + glowColor * glow;

    FragColor = vec4(color, 1.0);
}