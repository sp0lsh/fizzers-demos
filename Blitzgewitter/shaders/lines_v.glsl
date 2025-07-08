#version 330

uniform mat4 projection, modelview;

in vec4 vertex;

void main()
{
    gl_Position = projection * modelview * vertex;
}

