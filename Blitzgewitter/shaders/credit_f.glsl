#version 330

uniform sampler2D tex0;
uniform float alpha;

out vec4 output_colour;

in vec2 v2f_c;

void main()
{
    output_colour = texture2D(tex0,v2f_c);
    output_colour.a *= alpha;
}
