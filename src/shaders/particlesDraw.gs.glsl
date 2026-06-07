#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in ATTRIB_VS_OUT
{
    vec4 color;
    vec2 size;
    float zOrientation;
} attribIn[];

out ATTRIB_GS_OUT
{
    vec2 texCoords;
    vec4 color;
} attribOut;

uniform mat4 proj;

void main()
{
    vec4 center = gl_in[0].gl_Position;

    float halfWidth = attribIn[0].size.x * 0.5;
    float halfHeight = attribIn[0].size.y * 0.5;
    float angle = attribIn[0].zOrientation;

    float c = cos(angle);
    float s = sin(angle);
    mat2 rot = mat2(c, -s,
                    s,  c);

    vec2 baseOffsets[4] = vec2[](
        vec2(-halfWidth, -halfHeight),
        vec2( halfWidth, -halfHeight),
        vec2(-halfWidth,  halfHeight),
        vec2( halfWidth,  halfHeight)
    );

    vec2 uv[4] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );

    for (int i = 0; i < 4; ++i)
    {
        vec2 offset = rot * baseOffsets[i];

        vec4 viewPos = center + vec4(offset, 0.0, 0.0);
        gl_Position = proj * viewPos;

        attribOut.texCoords = uv[i];
        attribOut.color = attribIn[0].color;

        EmitVertex();
    }

    EndPrimitive();
}