#version 330 core

out vec4 fragColor;
const vec3 white = vec3(1.0, 1.0, 1.0);
uniform sampler2D rainMap;


in vec4 fragPosRainSpace1;
in float distAlpha1;

// taken from getShaow() in ex 8
float GetOcclusion(){

    vec3 projCoords = fragPosRainSpace1.xyz / fragPosRainSpace1.w;
    projCoords = projCoords * 0.5f + 0.5f;
    float closestDepth = texture(rainMap, projCoords.xy).r;
    float currentDepth = clamp(projCoords.z,-1.0f,1.0f);

    float bias =0.005;
    float isOccluded = currentDepth - bias > closestDepth  ? 0.0f : 0.3f;

    return isOccluded;
}

void main()
{
    float alpha = GetOcclusion()*distAlpha1;


    fragColor = vec4(white, alpha);
}
