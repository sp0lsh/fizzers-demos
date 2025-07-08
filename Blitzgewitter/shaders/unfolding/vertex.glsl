#version 330

uniform mat4 modelview, projection;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 colour;

out vec3 v2f_normal;
out vec3 v2f_e;
out vec3 v2f_colour;

void main()
{
    v2f_colour = colour;
    v2f_normal = normal;
    v2f_e = (modelview * vec4(pos, 1.0)).xyz;
    gl_Position = projection * modelview * vec4(pos, 1.0);
}

