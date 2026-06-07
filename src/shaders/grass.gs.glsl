#version 400 core

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

in ATTRIBS_TES_OUT
{
    vec3 position;
} attribsIn[];

out ATTRIBS_GS_OUT
{
    float height;
    float shimmer;
    vec3 worldPos;
} attribsOut;


uniform mat4 mvp;
uniform float time;
uniform vec3 cameraPos;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{        

    vec3 base = attribsIn[0].position;

    float dist = length(base.xz);
    float innerRadius = 3.5;
    float outerRadius = 4.5;

    float mask = smoothstep(innerRadius, outerRadius, dist);

    if (mask <= 0.01)
        return;

    float rotY = rand(base.xz) * 2.0 * 3.14159;
    float widthVar = rand(base.xz + 0.3);
    float heightVar = rand(base.xz + 0.7);
    float swayPhase = rand(base.xz + 1.3) * 6.28;

    float width = (0.02 + widthVar * 0.03) * mask;
    float height = (0.6 + heightVar * 0.8) * mask;

    float sway = sin(time * 1.5 + swayPhase) * 0.06;

    vec3 toCamera = cameraPos - base;
    toCamera.y = 0.0;
    
    if (length(toCamera) < 0.001) toCamera = vec3(0.0, 0.0, 1.0);
    else toCamera = normalize(toCamera);
    
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));

    width *= 2.0; // Billboards need to be slightly wider to feel as dense

    vec3 windOffset = vec3(sway, 0.0, sway);

    vec3 v0 = base - right * width * 0.5;
    vec3 v1 = base + right * width * 0.5;
    vec3 v2 = base + vec3(0.0, height, 0.0) + windOffset;

    attribsOut.height = 0.0; 
    attribsOut.shimmer = widthVar;
    attribsOut.worldPos = v0;
    gl_Position = mvp * vec4(v0, 1.0); 
    EmitVertex();

    attribsOut.height = 0.0;
    attribsOut.shimmer = widthVar;
    attribsOut.worldPos = v1;
    gl_Position = mvp * vec4(v1, 1.0);
    EmitVertex();

    attribsOut.height = 1.0;
    attribsOut.shimmer = widthVar;
    attribsOut.worldPos = v2;
    gl_Position = mvp * vec4(v2, 1.0);
    EmitVertex();

    EndPrimitive();    
}
