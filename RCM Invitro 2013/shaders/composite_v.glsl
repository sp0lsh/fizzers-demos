#version 330

in vec4 vertex;
in vec2 coord;

noperspective out vec2 v2f_coord;

void main()
{
    v2f_coord = coord;
    gl_Position = vertex;
}
