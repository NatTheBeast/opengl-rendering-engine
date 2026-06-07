#version 400 core

layout(location = 0) in vec3 position;

uniform mat4 mvp;

out vec2 groundPos;

float getTerrainHeight(vec2 pos)
{
    float h = sin(pos.x * 0.25) * cos(pos.y * 0.2) * 0.6;
    h += sin(pos.x * 0.05 + pos.y * 0.1) * 0.8;
    h += sin(pos.x * 0.8) * cos(pos.y * 0.8) * 0.1;
    float dist = length(pos);
    float flatten = clamp((dist - 3.0) / 4.0, 0.0, 1.0);
    return h * flatten;
}

void main()
{
    groundPos = position.xz;
    vec3 pos = position;
    pos.y += getTerrainHeight(pos.xz);
    gl_Position = mvp * vec4(pos, 1.0);
}