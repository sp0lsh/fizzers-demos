#version 330

uniform mat4 modelview;

in vec4 vertex;

out vec3 v2g_o;

void main()
{
    v2g_o = vertex.xyz;
    gl_Position = modelview * vertex;
}
