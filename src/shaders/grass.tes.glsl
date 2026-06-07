#version 400 core

layout(triangles, equal_spacing, cw, point_mode) in;

out ATTRIBS_TES_OUT
{
    vec3 position;
} attribsOut;

uniform mat4 mvp;

float getTerrainHeight(vec2 pos)
{
    float h = sin(pos.x * 0.25) * cos(pos.y * 0.2) * 0.6;
    h += sin(pos.x * 0.05 + pos.y * 0.1) * 0.8;
    h += sin(pos.x * 0.8) * cos(pos.y * 0.8) * 0.1;
    // Flatten the center where objects are (radius 3.0)
    float dist = length(pos);
    float flatten = clamp((dist - 3.0) / 4.0, 0.0, 1.0);
    return h * flatten;
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec4 position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
    
    // Jitter positions to break the grid pattern
    float jitterX = (rand(position.xz) - 0.5) * 1.5;
    float jitterZ = (rand(position.zx) - 0.5) * 1.5;
    position.x += jitterX;
    position.z += jitterZ;

    // Displace height
    position.y += getTerrainHeight(position.xz);

    attribsOut.position = position.xyz;

    gl_Position = position;
}
