#version 330

uniform sampler2D tex;
uniform vec3 colour;

in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    output_colour.a = 1.0;
    output_colour.rgb = texture2D(tex, v2f_coord).rgb;
}
