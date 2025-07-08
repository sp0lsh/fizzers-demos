#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 projection;

in vec3 v2g_o[3];
in vec3 v2g_vn[3];

out vec3 g2f_n;
out vec3 g2f_v;
out vec3 g2f_o;

void main()
{
    gl_Position=projection*gl_in[0].gl_Position;
    g2f_n=v2g_vn[0];
    g2f_v=gl_in[0].gl_Position.xyz;
    g2f_o=v2g_o[0];
    EmitVertex();

    gl_Position=projection*gl_in[1].gl_Position;
    g2f_n=v2g_vn[1];
    g2f_v=gl_in[1].gl_Position.xyz;
    g2f_o=v2g_o[1];
    EmitVertex();

    gl_Position=projection*gl_in[2].gl_Position;
    g2f_n=v2g_vn[2];
    g2f_v=gl_in[2].gl_Position.xyz;
    g2f_o=v2g_o[2];
    EmitVertex();

    EndPrimitive();
}
