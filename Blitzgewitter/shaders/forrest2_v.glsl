#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec3 normal;

out vec3 v2f_n;
out vec3 v2f_o;

out vec4 w_pos;

noperspective out vec2 v2f_screen;
noperspective out vec2 v2f_zw;

void main()
{
    v2f_o = vertex.xyz;
    v2f_n = mat3(modelview) * normal;
    
    gl_Position = projection * modelview * vertex;
	w_pos = gl_Position;
    v2f_zw = gl_Position.zw;
    v2f_screen = gl_Position.xy / gl_Position.w;
}
