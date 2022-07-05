#version 330 core

uniform vec3 camPosition; // so we can compute the view vector
out vec4 FragColor; // the output color of this fragment

// light uniform variables
uniform vec4 ambientLightColor;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightRadius;

// material properties
uniform vec3 reflectionColor;
uniform float roughness;
// legacy uniforms, not needed for PBR
uniform float ambientReflectance;
uniform float diffuseReflectance;
uniform float specularReflectance;
uniform float specularExponent;

// material textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_ambient1;
uniform sampler2D texture_specular1;
uniform samplerCube skybox;
uniform sampler2D shadowMap;


// 'in' variables to receive the interpolated Position and Normal from the vertex shader
in vec4 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec2 textureCoordinates;


in vec4 fragPosLightSpace;


// Rain
uniform sampler2D rainMap;
in vec4 fragPosRainSpace;


// Constant Pi
const float PI = 3.14159265359;

float resultRoughness = 0.0f;


float GetWetness()
{

   vec3 projCoords = fragPosRainSpace.xyz / fragPosRainSpace.w;
   projCoords = projCoords * 0.5 + 0.5;

   float closestDepth = texture(rainMap, projCoords.xy).r;
   float currentDepth = clamp(projCoords.z,-1,1);

   float bias =0.005;
   float wetness = currentDepth - bias > closestDepth  ? 0.0f : 1.0f;

   // source: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
   vec2 texelSize = 1.0 / textureSize(rainMap, 0);
   for(int x = -1; x <= 1; ++x)
   {
      for(int y = -1; y <= 1; ++y)
      {
         float pcfDepth = texture(rainMap, projCoords.xy + vec2(x, y) * texelSize).r;
         wetness += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
      }
   }

   return wetness/=15.0f;
}



vec3 FresnelSchlick(vec3 F0, float cosTheta)
{

   vec3 schlick = F0 + (1-F0)*pow(1-cosTheta,5);

   return schlick;
}

float DistributionGGX(vec3 N, vec3 H, float a)
{

   float over = pow(a,2);
   float under = (PI * pow( pow( max(dot(N,H),0.0),2) * (pow(a,2)-1)+1,2));

   float ggx= over / under;
   return ggx;
}

float GeometrySchlickGGX(float cosAngle, float a)
{
   float over = 2*cosAngle;
   float under = cosAngle + sqrt(pow(a,2)+(1+pow(a,2)*pow(cosAngle,2)));

   float ggx= over / under;

   return ggx;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float a)
{
   float incomming = GeometrySchlickGGX(max(dot(N,L),0.0),a);
   float outgoing = GeometrySchlickGGX(max(dot(N,V),0.0),a);


   return incomming*outgoing;
}




vec3 GetCookTorranceSpecularLighting(vec3 N, vec3 L, vec3 V)
{
   vec3 H = normalize(L + V);


   // Get the rougness from a texture to use for Fresnel term
   float roughnessTexture = 0.5f;//texture(texture_ambient1, textureCoordinates).g;

   resultRoughness = mix(roughnessTexture, 0.01f, GetWetness());
   float a = resultRoughness * resultRoughness;

   float D = DistributionGGX(N, H, a);
   float G = GeometrySmith(N, V, L, a);

   float cosI = max(dot(N, L), 0.0);
   float cosO = max(dot(N, V), 0.0);

   // Important! Notice that Fresnel term (F) is not here because we apply it later when mixing with diffuse
   float specular = (D * G) / (4.0f * cosO * cosI + 0.0001f);

   return vec3(specular);
}



vec3 GetNormalMap()
{
   //NEW! Normal map

   // Sample normal map
   vec3 normalMap = texture(texture_normal1, textureCoordinates).rgb;
   // Unpack from range [0, 1] to [-1 , 1]
   normalMap = normalMap * 2.0 - 1.0;

   // This time we are storing Z component in the texture, no need to compute it. Instead we normalize just in case
   normalMap = normalize(normalMap);

   // Create tangent space matrix
   vec3 N = normalize(worldNormal);
   vec3 B = normalize(cross(worldTangent, N)); // Orthogonal to both N and T
   vec3 T = cross(N, B); // Orthogonal to both N and B. Since N and B are normalized and orthogonal, T is already normalized
   mat3 TBN = mat3(T, B, N);

   // Transform normal map from tangent space to world space
   return TBN * normalMap;
}

vec3 GetAmbientLighting(vec3 albedo, vec3 normal)
{

   vec3 ambient = textureLod(skybox, normal,6.0f).xyz;
   ambient *= albedo/ PI;

   float ambientOcclusion = texture(texture_ambient1, textureCoordinates).r;
   ambient *= ambientOcclusion;

   return ambient;
}

vec3 GetEnvironmentLighting(vec3 N, vec3 V)
{

   vec3 R = reflect(-V, N);

   vec3 reflection = textureLod(skybox, R, resultRoughness * 5.0f).rgb;

   reflection = mix(reflection, reflection*max(dot(N,V),0), resultRoughness);

   return reflection;
}

vec3 GetLambertianDiffuseLighting(vec3 N, vec3 L, vec3 albedo)
{
   vec3 diffuse = albedo;

   diffuse = diffuse/PI;

   return diffuse;
}

vec3 GetBlinnPhongSpecularLighting(vec3 N, vec3 L, vec3 V)
{
   vec3 H = normalize(L + V);

   float specModulation = pow(max(dot(H, N), 0.0), specularExponent);
   vec3 specular = vec3(specularReflectance) * specModulation;

   return specular;
}

float GetAttenuation(vec4 P)
{
   float distToLight = distance(lightPosition, P.xyz);
   float attenuation = 1.0f / (distToLight * distToLight);

   float falloff = smoothstep(lightRadius, lightRadius*0.5f, distToLight);

   return attenuation * falloff;
}

float GetShadow()
{

   vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
   projCoords = projCoords * 0.5 + 0.5;

   float closestDepth = texture(shadowMap, projCoords.xy).r;
   float currentDepth = clamp(projCoords.z,-1,1);

   float bias =0.005;
   float shadow = currentDepth - bias > closestDepth  ? 0.0 : 1.0;

   return shadow;
}

// The fuction for wetness an approximation of a function by Sébastien Lagrede
vec3 GetWetAlbedeo(vec3 dryAlbedeo){

   vec3 a = -2.5f * (dryAlbedeo*dryAlbedeo*dryAlbedeo);
   vec3 b = 5.04f * (dryAlbedeo*dryAlbedeo);
   vec3 c = -2.56f*dryAlbedeo;

   return a + b + c + 1.0f;

}

void main()
{
   vec4 P = worldPos;
   vec3 N = GetNormalMap();

   // Uses the textures to get the correct wetness
   float wetness = GetWetness();

   float metalness = 0;//texture(texture_ambient1, textureCoordinates).b;
   vec3 albedo = texture(texture_diffuse1, textureCoordinates).xyz;

   vec3 albedoWet = albedo*GetWetAlbedeo(albedo);

   albedo =mix(mix(albedo, albedoWet, wetness),albedo,metalness );
   albedo *= reflectionColor;


   bool positional = lightRadius > 0;

   vec3 L = normalize(lightPosition - (positional ? P.xyz : vec3(0.0f)));
   vec3 V = normalize(camPosition - P.xyz);

   vec3 ambient = GetAmbientLighting(albedo, N);
   vec3 diffuse = GetLambertianDiffuseLighting(N, L, albedo);
   vec3 environment = GetEnvironmentLighting(N, V);

   vec3 specular = GetCookTorranceSpecularLighting(N,L,V);

   // This time we get the lightColor outside the diffuse and specular terms (we are multiplying later)
   vec3 lightRadiance = lightColor;

   // Modulate lightRadiance by distance attenuation (only for positional lights)
   float attenuation = positional ? GetAttenuation(P) : 1.0f;
   lightRadiance *= attenuation;

   // Modulate lightRadiance by shadow (only for directional light)
   float shadow = positional ? 1.0f : GetShadow();
   lightRadiance *= shadow;

   // Modulate the radiance with the angle of incidence
   lightRadiance *= max(dot(N, L), 0.0);

   lightRadiance = max(lightRadiance, 0);
   // We use a fixed value of 0.04f for F0. The range in dielectrics is usually in the range (0.02, 0.05)
   vec3 F0 = vec3(0.04f);


   F0 = mix(F0,albedo,metalness);

   diffuse  = mix(diffuse,vec3(0),metalness);
   ambient = mix(ambient,vec3(0),metalness);

   // Compute the Fresnel term for indirect light,
   // using the clamped cosine of the angle formed by
   // the NORMAL vector and the view vector
   vec3 H = normalize(L + V);

   vec3 schlickAmbient = FresnelSchlick( F0,max(dot(V, N), 0.0));
   vec3 indirectLight = mix(ambient,environment,schlickAmbient);
   vec3 schlickSpec = FresnelSchlick( F0,max(dot(V, H), 0.0));


   vec3 directLight = mix(diffuse,specular,schlickSpec);
   directLight *= lightRadiance;

   // lighting = indirect lighting (ambient + environment) + direct lighting (diffuse + specular)
   vec3 lighting = directLight + indirectLight;

   FragColor = vec4(lighting, 1.0f);
}
