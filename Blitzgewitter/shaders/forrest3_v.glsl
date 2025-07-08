#version 330

uniform mat4 projection, modelview;
uniform vec2 jitter;

in vec4 vertex;
in vec3 normal;
in vec2 coord;

out vec3 v2f_v, v2f_n;
out vec3 v2f_o;
out vec2 v2f_coord;

noperspective out vec2 v2f_screen;

void main()
{
    v2f_coord = coord;

    v2f_o = vertex.xyz;
    v2f_v = (modelview * vertex).xyz;
    v2f_n = mat3(modelview) * normal;
    
    gl_Position = projection * vec4(v2f_v, 1.0);
    v2f_screen = gl_Position.xy / gl_Position.w + jitter;
}
