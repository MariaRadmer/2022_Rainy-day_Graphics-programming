#version 330 core
layout (location = 0) in vec3 pos;

uniform float currentTime;
uniform float boxSize;
uniform vec3 cameraPosition;
uniform vec3 forward;
uniform vec3 offsets;
uniform vec3 velocity;

// for splash and occlusion
uniform mat4 rainSpaceMatrix;
uniform mat4 model;

out vec4 fragPosRainSpace;
out float distAlpha;

void main()
{

   float elapsedTimeFrag = currentTime;
   vec3 worldPos =pos;

   vec3 vel = velocity * elapsedTimeFrag;
   vel -= cameraPosition + forward + vec3(boxSize)*0.5f;

   vec3 position = pos+ vel;
   position = mod(position, boxSize);

   worldPos = position + cameraPosition +forward- (vec3(boxSize)*0.5f);

   vec3 top = worldPos.xyz -velocity;
   vec3 bottom = worldPos.xyz;
   float distanceTopBottom = length(top.xy-bottom.xy);
   distAlpha= 1.0f-distanceTopBottom/boxSize*0.5f;

   position= mix(top, bottom, mod(gl_VertexID, 2.0f));


   fragPosRainSpace= rainSpaceMatrix* vec4(worldPos,1.0f);
   gl_Position = vec4(worldPos,1.0f);
}