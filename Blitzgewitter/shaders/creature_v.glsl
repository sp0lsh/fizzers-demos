#version 330

uniform mat4 projection, modelview;

layout(location=0) in vec4 vertex;
layout(location=2) in vec2 coord;

out vec3 v2f_o;

void main()
{
    v2f_o = vertex.xyz;
    gl_Position = projection * modelview * vertex;
}
