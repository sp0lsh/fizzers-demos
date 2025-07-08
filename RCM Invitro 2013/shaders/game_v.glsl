#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec2 coord;

out vec2 v2f_coord;

void main()
{
    v2f_coord = coord;
    gl_Position = projection * modelview * vertex;
}
