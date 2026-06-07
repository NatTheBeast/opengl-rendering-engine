#version 400 core

in vec2 uv;
out vec4 FragColor;

uniform vec2 sunScreenPos;
uniform float aspectRatio;
uniform vec3 sunColor;
uniform float time;
uniform int renderMode; // 0 = sun glow only, 1 = lens artifacts only

float sdHexagon(vec2 p, float r) {
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}

void main() {
    // Current pixel shifted to aspect-ratio-corrected space (center is 0,0)
    vec2 p = uv - 0.5;
    p.x *= aspectRatio;
    
    // Sun position mapped on screen (also aspect-corrected)
    vec2 sp = sunScreenPos - 0.5;
    sp.x *= aspectRatio;

    vec2 toSun = p - sp;
    float distToSun = length(toSun);
    vec2 dirToSun = normalize(toSun);
    
    vec3 color = vec3(0.0);
    
    // Warmer lens flare color
    vec3 warmFlareColor = mix(sunColor, vec3(1.0, 0.5, 0.1), 0.6);
    
    // 1. MAIN GLOW (sun only — mode 0)
    if (renderMode == 0) {
        // Soft glow near the center, decays quickly
        float glow = 0.015 / (distToSun + 0.02);
        glow *= exp(-distToSun * 2.5);
        color += warmFlareColor * glow * 0.8;

        // 2. STARBURST
        // Radial noise rays originating from the sun
        float angle = atan(toSun.y, toSun.x);
        float rays1 = fract(sin(floor(angle * 30.0)) * 123.456);
        float rays2 = fract(sin(floor(angle * 12.0 + 3.14)) * 987.654);
        float rays = (rays1 * 0.5 + rays2 * 0.5);
        // Attenuate starburst so it's only visible near the sun
        rays *= exp(-distToSun * 3.5) * 0.8;
        color += mix(warmFlareColor, vec3(1.0, 0.8, 0.6), 0.5) * rays * glow;
    }

    // 3. RING (lens artifact — mode 1)
    if (renderMode == 1) {
        // Halo ring that appears between center and screen edge
        float ringRadius = 0.35;
        float ringDist = abs(length(p) - ringRadius);
        float ringIntensity = smoothstep(0.03, 0.0, ringDist);
        // It's brighter in the direction traversing from the center to the sun
        float ringDir = dot(normalize(p), normalize(sp));
        float ringGlow = smoothstep(-0.2, 1.0, ringDir);
        color += warmFlareColor * ringIntensity * ringGlow * 0.6;

        // 4. HEXAGON GHOSTS
        // Render several hexagons that bounce through the center to the opposite side of the screen
        const int NUM_GHOSTS = 5;
        float ghostScales[NUM_GHOSTS] = float[](0.04, 0.08, 0.02, 0.12, 0.05);
        float ghostPositions[NUM_GHOSTS] = float[](-0.4, 0.3, 0.7, -0.8, -1.2);
        vec3 ghostColors[NUM_GHOSTS] = vec3[](
            vec3(0.5, 0.7, 1.0),
            vec3(0.9, 0.8, 0.4),
            vec3(0.4, 1.0, 0.5),
            vec3(0.2, 0.4, 1.0),
            vec3(0.8, 0.3, 0.3)
        );
        
        for (int i = 0; i < NUM_GHOSTS; i++) {
            vec2 gp = ghostPositions[i] * sp; // Vector mapped along the flare axis
            vec2 localToGhost = p - gp;
            
            float d = sdHexagon(localToGhost, ghostScales[i]);
            
            // Anti-aliased outer edge
            float ghostIntensity = smoothstep(0.005, 0.0, d);
            
            // Emptied center (gives it a glass reflection look)
            ghostIntensity *= smoothstep(-ghostScales[i] * 0.6, -ghostScales[i] * 0.2, d);
            
            color += ghostColors[i] * ghostIntensity * 0.5;
        }

        // 5. CAUSTIC BLOB
        // A soft colorful blob on the opposite side
        vec2 causticPos = -sp * 1.4;
        float causticDist = length(p - causticPos);
        float causticBlob = 0.02 / (causticDist + 0.01);
        causticBlob *= exp(-causticDist * 3.0);
        color += vec3(0.3, 0.5, 1.0) * causticBlob * 1.5;
    }

    // Fade EVERYTHING out if the sun leaves the center area (approaches screen edge)
    // Calculate raw distance in UV screen coordinates (before aspect ratio correction)
    vec2 uvDist = abs(sunScreenPos - 0.5);
    float maxUVDist = max(uvDist.x, uvDist.y); // If > 0.5, sun is theoretically off-screen
    // Allow the flare to persist slightly off-screen (up to 0.7) for a smoother gradient
    float screenFade = 1.0 - smoothstep(0.3, 0.8, maxUVDist);

    FragColor = vec4(color * screenFade, 1.0);
}
