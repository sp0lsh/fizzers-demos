#version 330

uniform vec3 colour;

in vec3 v2f_o;

out vec4 output_colour0;

void main()
{
    output_colour0.a = 1.0;
    output_colour0.rgb = colour * vec3(0.4);
}
