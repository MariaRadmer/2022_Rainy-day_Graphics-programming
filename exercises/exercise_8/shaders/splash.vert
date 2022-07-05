#version 330 core
layout (location = 0) in vec3 pos;

uniform float currentTime;
uniform float boxSize;
uniform vec3 cameraPosition;
uniform vec3 forward;
uniform vec3 offsets;
uniform vec3 velocity;
uniform float deltaTime;
uniform float splashSpeed;

// for splash and occlusion
uniform mat4 rainSpaceMatrix;
uniform mat4 model;
uniform sampler2D rainMap;

out float vDepth;
out float vOpacity;

void main()
{
    float elapsedTimeFrag = currentTime;

    // --- Taken inspiration from Charlie Birtwistle and Stephen Mcauley code in 5.1 Dynamic Weather Effects
    vec3 worldPos =pos;
    vec3 vel = velocity* elapsedTimeFrag;
    vel -= cameraPosition + forward + vec3(boxSize)*0.5f;
    worldPos += vel;
    worldPos = mod(worldPos, boxSize);

    worldPos = worldPos + cameraPosition +forward- (vec3(boxSize)*0.5f);
    //---

    vec4 fragPosRainSpace= rainSpaceMatrix* vec4(worldPos,1.0f);
    vec3 projCoords = fragPosRainSpace.xyz / fragPosRainSpace.w;

    projCoords = projCoords * 0.5f + 0.5f;
    float closestDepth = texture(rainMap, projCoords.xy).r;
    float currentDepth = clamp(projCoords.z,-1.0f,1.0f);

    projCoords.z = closestDepth;
    projCoords= projCoords*2-1;

    vec4 temp = inverse(rainSpaceMatrix)*vec4(projCoords,1.0);
    vec3 splashPos=  temp.xyz/temp.w;


    float range = clamp((closestDepth-currentDepth)/splashSpeed, 0, 1);
    vDepth =  mix(0.2,0.0f,range);


    gl_Position = vec4(splashPos,1.0f);
}