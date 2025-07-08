#version 330

uniform mat4 modelview;
uniform mat4 projection;

in vec4 vertex;
in vec2 coord;

out vec3 v2f_o;
out vec2 v2f_c;

void main()
{
    v2f_o = vertex.xyz;
    v2f_c = coord;
    gl_Position = modelview * vertex;
}
