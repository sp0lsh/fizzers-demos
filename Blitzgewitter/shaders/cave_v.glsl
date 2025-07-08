#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec2 coord;
in vec3 normal;

out vec3 v2f_n;
out vec3 v2f_o;
out vec2 v2f_coord;

void main()
{
    v2f_coord = coord;

    v2f_o = vertex.xyz;
    v2f_n = normal;
    
    gl_Position = projection * modelview * vertex;
}
