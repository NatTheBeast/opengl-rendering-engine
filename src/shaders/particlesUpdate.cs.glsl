#version 430 core

layout(local_size_x = 1) in;

struct Particle
{
    vec3 position;
    float zOrientation;
    vec4 velocity;
    vec4 color;
    vec2 size;
    float timeToLive;
    float maxTimeToLive;
};

layout(std430, binding = 0) readonly buffer ParticlesInputBlock
{
    Particle particles[];
} dataIn;

layout(std430, binding = 1) writeonly buffer ParticlesOutputBlock
{
    Particle particles[];
} dataOut;

uniform float time;
uniform float deltaTime;
uniform vec3 emitterPosition;
uniform vec3 emitterDirection;

const float PI = 3.14159265359;

// random simple
float rand01(float seed)
{
    return fract(sin(seed) * 43758.5453123);
}

void main()
{
    uint id = gl_GlobalInvocationID.x;

    Particle p = dataIn.particles[id];

    // SI PARTICULE MORTE → RESET
    if (p.timeToLive <= 0.0)
    {
        float s0 = float(id) * 17.0 + time * 13.0;
        float s1 = float(id) * 31.0 + time * 19.0;
        float s2 = float(id) * 47.0 + time * 23.0;
        float s3 = float(id) * 59.0 + time * 29.0;

        float angle = rand01(s0) * 2.0 * PI;

        vec3 dir = normalize(emitterDirection);
        vec3 side = normalize(vec3(-dir.z, 0.0, dir.x));

        vec3 velocity =
            dir * (0.3 + rand01(s1)*0.2) +
            vec3(0.0, 0.2 + rand01(s2)*0.2, 0.0) +
            side * (rand01(s3)*0.2 - 0.1);

        p.position = emitterPosition;
        p.zOrientation = angle;
        p.velocity = vec4(velocity, 0.0);

        p.timeToLive = 1.5 + rand01(s0 + 10.0);
        p.maxTimeToLive = p.timeToLive;

        p.size = vec2(0.2);
        p.color = vec4(0.5, 0.5, 0.5, 0.0);
    }
    else
    {
        // update vie
        p.timeToLive -= deltaTime;

        if (p.timeToLive < 0.0)
            p.timeToLive = 0.0;

        float life = 1.0 - (p.timeToLive / p.maxTimeToLive);

        // Euler
        p.position += p.velocity.xyz * deltaTime;

        // rotation
        p.zOrientation += deltaTime;

        // couleur gris → blanc
        vec3 col = mix(vec3(0.5), vec3(1.0), life);

        // fade in/out
        float fadeIn = smoothstep(0.0, 0.2, life);
        float fadeOut = 1.0 - smoothstep(0.8, 1.0, life);
        float alpha = fadeIn * fadeOut;

        p.color = vec4(col, alpha);

        // taille augmente
        float s = mix(0.2, 0.5, life);
        p.size = vec2(s);
    }

    dataOut.particles[id] = p;
}