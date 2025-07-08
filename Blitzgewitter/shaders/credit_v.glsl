#version 330

in vec4 vertex;
in vec2 coord;

out vec2 v2f_c;

void main()
{
    v2f_c = coord;
    gl_Position = vertex;
}
