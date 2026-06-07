#version 410 core

out vec3 viewDir;

uniform mat4 invProj;
uniform mat3 invViewRot;
uniform float fovScale; // >1 = sky feels farther, <1 = closer

void main()
{
    // Fullscreen triangle from gl_VertexID (no VBO needed)
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vec4 clipPos = vec4(pos * 2.0 - 1.0, 1.0, 1.0);
    gl_Position = clipPos;

    // Scale clip-space xy to widen effective FOV for the sky
    vec4 scaledClip = vec4(clipPos.xy * fovScale, clipPos.zw);

    // Reconstruct world-space view direction
    vec4 viewSpace = invProj * scaledClip;
    viewDir = invViewRot * viewSpace.xyz;
}
