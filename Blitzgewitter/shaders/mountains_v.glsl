#version 330

uniform mat4 projection, modelview;

in vec4 vertex;

out vec3 v2f_v;
out vec3 v2f_o;

void main()
{
    v2f_o = vertex.xyz;
    v2f_v = (modelview * vertex).xyz;
    
    gl_Position = projection * vec4(v2f_v, 1.0);
}
