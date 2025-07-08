#version 330

uniform mat4 projection, modelview;

in vec4 vertex;

out vec3 v2f_o;

void main()
{
    v2f_o = vertex.xyz;
    gl_Position = projection * modelview * vertex;
}
