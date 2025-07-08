#version 330

uniform mat4 modelview;
uniform float time;

in vec4 vertex;

out int vid;

void main()
{
    vid = gl_VertexID;
    gl_Position = modelview * (vertex * vec4(vec3(0.5+sqrt(time*pow(1.0+0.75*sin(float(vid)*3.3),2.0))),1.0));
}

