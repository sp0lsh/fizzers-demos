#version 330

uniform sampler2D tex;
uniform vec3 colour;
uniform float time;

in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    output_colour.a = texture2D(tex, v2f_coord).r;
    output_colour.rgb = colour;
}
