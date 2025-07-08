#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec2 coord;

out vec2 coord_v2f;

void main()
{
    coord_v2f = coord;
    gl_Position = projection * modelview * vertex;
}

