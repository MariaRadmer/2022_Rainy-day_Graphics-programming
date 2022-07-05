#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float splashQuadSize;
uniform vec3 velocity;
uniform mat4 viewProjection;

in float vDepth[];

out float gDepth;
out vec2 textCoords;

void main() {
    gl_Position = viewProjection*(gl_in[0].gl_Position + vec4(splashQuadSize,splashQuadSize,0.0,0.0));
    gDepth = vDepth[0];
    textCoords = vec2(0,0);
    EmitVertex();

    gl_Position = viewProjection*(gl_in[0].gl_Position+ vec4(splashQuadSize,-splashQuadSize,0.0,0.0));
    gDepth = vDepth[0];
    textCoords = vec2(0,1);
    EmitVertex();

    gl_Position = viewProjection*(gl_in[0].gl_Position+ vec4(-splashQuadSize,splashQuadSize,0.0,0.0));
    gDepth = vDepth[0];
    textCoords = vec2(1,0);
    EmitVertex();


    gl_Position = viewProjection*(gl_in[0].gl_Position+ vec4(-splashQuadSize,-splashQuadSize,0.0,0.0));
    gDepth = vDepth[0];
    textCoords = vec2(1,1);
    EmitVertex();

    EndPrimitive();
}