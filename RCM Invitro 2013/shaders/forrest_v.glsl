#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec3 normal;
in vec2 coord;

out vec3 v2f_v /* view vector */, v2f_n /* normal */, v2f_l1, v2f_l2, v2f_l3;
out vec3 v2f_o; // vertex in object space
out vec2 v2f_coord;

noperspective out vec2 v2f_screen;
noperspective out vec2 v2f_zw;

void main()
{
    v2f_coord = coord;

    v2f_o = vertex.xyz;
    v2f_v = (modelview * vertex).xyz;
    v2f_n = mat3(modelview) * normal;

    v2f_l1 = (mat3(modelview)) * normalize(vec3(1.0, -0.01, 1.0)); // lights
    v2f_l2 = (mat3(modelview)) * normalize(vec3(1.0, -0.01, 0.1));
    v2f_l3 = (mat3(modelview)) * normalize(vec3(0.0, -1.0, -0.1));
    
    gl_Position = projection * vec4(v2f_v, 1.0);
    v2f_zw = gl_Position.zw;
    v2f_screen = gl_Position.xy / gl_Position.w;
}
