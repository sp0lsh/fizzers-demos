#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
layout(location = 1) in vec3 colour;

out vec3 v2g_c;

void main()
{
    v2g_c = colour;
    gl_Position = projection * modelview * vertex;
}

