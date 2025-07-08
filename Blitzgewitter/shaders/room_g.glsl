#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 projection;

in vec3 v2g_o[3];

out vec3 g2f_n;
out vec3 g2f_v;
out vec3 g2f_o;
out vec3 g2f_bcoord;

void main()
{
    vec3 norm=normalize(cross(gl_in[1].gl_Position.xyz-gl_in[0].gl_Position.xyz,gl_in[2].gl_Position.xyz-gl_in[0].gl_Position.xyz));

    gl_Position=projection*gl_in[0].gl_Position;
    g2f_n=norm;
    g2f_v=gl_in[0].gl_Position.xyz;
    g2f_o=v2g_o[0];
    g2f_bcoord=vec3(1.0,0.0,0.0);
    EmitVertex();

    gl_Position=projection*gl_in[1].gl_Position;
    g2f_n=norm;
    g2f_v=gl_in[1].gl_Position.xyz;
    g2f_o=v2g_o[1];
    g2f_bcoord=vec3(0.0,1.0,0.0);
    EmitVertex();

    gl_Position=projection*gl_in[2].gl_Position;
    g2f_n=norm;
    g2f_v=gl_in[2].gl_Position.xyz;
    g2f_o=v2g_o[2];
    g2f_bcoord=vec3(0.0,0.0,1.0);
    EmitVertex();

    EndPrimitive();
}
