#version 330

uniform sampler2D tex0;
uniform float time, scale;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    output_colour = texture2D(tex0, v2f_coord) * scale;
}

