#version 400 core

layout(vertices = 3) out;

uniform mat4 modelView;

void main()
{
    
    if(gl_InvocationID == 0)
    {
        vec4 mid0 = modelView * mix(gl_in[0].gl_Position, gl_in[1].gl_Position, 0.5);
        vec4 mid1 = modelView * mix(gl_in[1].gl_Position, gl_in[2].gl_Position, 0.5);
        vec4 mid2 = modelView * mix(gl_in[2].gl_Position, gl_in[0].gl_Position, 0.5);

        const float MIN_TESS = 2.0;
        const float MAX_TESS = 6.0; // Reduced from 16 to fix severe software rendering fallbacks on Mac
        const float MIN_DIST = 10.0;
        const float MAX_DIST = 60.0;
        const float CULL_DIST = 90.0;

        float dist0 = length(mid0.xyz);
        float dist1 = length(mid1.xyz);
        float dist2 = length(mid2.xyz);

        if (dist0 > CULL_DIST && dist1 > CULL_DIST && dist2 > CULL_DIST) {
            gl_TessLevelOuter[0] = 0.0;
            gl_TessLevelOuter[1] = 0.0;
            gl_TessLevelOuter[2] = 0.0;
            gl_TessLevelInner[0] = 0.0;
        } else {
            dist0 = clamp(dist0, MIN_DIST, MAX_DIST);
            dist1 = clamp(dist1, MIN_DIST, MAX_DIST);
            dist2 = clamp(dist2, MIN_DIST, MAX_DIST);

            float tess0 = mix(MAX_TESS, MIN_TESS, (dist0 - MIN_DIST) / (MAX_DIST - MIN_DIST));
            float tess1 = mix(MAX_TESS, MIN_TESS, (dist1 - MIN_DIST) / (MAX_DIST - MIN_DIST));
            float tess2 = mix(MAX_TESS, MIN_TESS, (dist2 - MIN_DIST) / (MAX_DIST - MIN_DIST));

            gl_TessLevelOuter[0] = tess0;
            gl_TessLevelOuter[1] = tess1;
            gl_TessLevelOuter[2] = tess2;

            gl_TessLevelInner[0] = max(tess0, max(tess1, tess2));
        }

    }
    
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
