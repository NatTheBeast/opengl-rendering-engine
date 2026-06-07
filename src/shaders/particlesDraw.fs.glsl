#version 330 core

in ATTRIB_GS_OUT
{
    vec2 texCoords;
    vec4 color;
} attribIn;

out vec4 FragColor;

uniform sampler2D textureSampler;

void main()
{
    vec4 texel = texture(textureSampler, attribIn.texCoords);

    if (texel.a < 0.02)
        discard;

    FragColor = texel * attribIn.color;
}