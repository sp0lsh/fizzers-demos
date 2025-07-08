#version 330

uniform mat4 modelview;

in vec4 vertex;
layout(location=3) in vec3 vertex_normal;

out vec3 v2g_o;
out vec3 v2g_vn;

void main()
{
    v2g_o = vertex.xyz;
    v2g_vn = inverse(transpose(mat3(modelview))) * vertex_normal;
    gl_Position = modelview * vertex;
}
