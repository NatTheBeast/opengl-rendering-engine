#version 400 core

out vec2 uv;

void main()
{
    // Generates a full screen triangle without any VBO bound
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    
    uv.x = (x + 1.0) * 0.5;
    uv.y = (y + 1.0) * 0.5;
    
    // Draw the flare at the far plane so objects in front (grass, ground) occlude it
    gl_Position = vec4(x, y, 0.9999f, 1.0);
}
