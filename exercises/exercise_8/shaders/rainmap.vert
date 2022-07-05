#version 330 core
layout (location = 0) in vec3 vertex;

uniform mat4 rainSpaceMatrix;
uniform mat4 model;

out vec4 fragPosRainSpace;

void main()
{

   gl_Position = rainSpaceMatrix*model * vec4(vertex, 1.0);

}