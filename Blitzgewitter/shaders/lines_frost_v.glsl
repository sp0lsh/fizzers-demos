#version 330

uniform mat4 projection, modelview;
uniform float clip_side;

in vec4 vertex;

out float v2g_cd;

void main()
{
    v2g_cd = vertex.x*clip_side;
    gl_Position = projection * modelview * vertex;
}

