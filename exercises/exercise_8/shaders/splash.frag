#version 330 core

out vec4 fragColor;
const vec3 white = vec3(0.9, 0.9, 1.0);
const vec3 blue = vec3(0.0, 0.0, 1.0);
uniform sampler2D rainMap;
uniform sampler2D splashTexture;


in float gDepth;
in vec2 textCoords;

void main()
{

    float texture = texture(splashTexture, textCoords.xy).r;
    fragColor = vec4(white,gDepth*texture);
}
