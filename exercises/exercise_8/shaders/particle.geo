#version 330 core
layout (points) in;
layout (line_strip, max_vertices = 2) out;

uniform vec3 length;
uniform vec3 velocity;
uniform mat4 viewProjection;

in vec4 fragPosRainSpace[];
in float distAlpha[];

out vec4 fragPosRainSpace1;
out float distAlpha1;

void main() {
    gl_Position = viewProjection*gl_in[0].gl_Position;
    fragPosRainSpace1 = fragPosRainSpace[0];
    distAlpha1 = distAlpha[0];
    EmitVertex();

    gl_Position = viewProjection*(gl_in[0].gl_Position - vec4(velocity,0.0));
    fragPosRainSpace1 = fragPosRainSpace[0];
    distAlpha1 = distAlpha[0];
    EmitVertex();

    EndPrimitive();
}