#version 330

uniform mat4 projection, modelview, object;

in vec4 vertex;
in vec3 normal;
in vec2 coord;

out vec3 v2f_v, v2f_n;
out vec3 v2f_o;
out vec2 v2f_coord;

out vec4 w_pos;

noperspective out vec2 v2f_screen;
noperspective out vec2 v2f_zw;

void main()
{
    v2f_coord = coord;

    v2f_o = (object * vertex).xyz;
    v2f_v = (modelview * object * vertex).xyz;
    v2f_n = mat3(modelview) * normal;
    
    gl_Position = projection * vec4(v2f_v, 1.0);
	w_pos = gl_Position;
    v2f_zw = gl_Position.zw;
    v2f_screen = gl_Position.xy / gl_Position.w;
}
