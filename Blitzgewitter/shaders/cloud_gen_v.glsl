#version 330

in vec4 vertex;

noperspective out vec2 v2f_coord;

void main()
{
    v2f_coord = vertex.xy * 0.5 + vec2(0.5);
    gl_Position = vertex;
}
