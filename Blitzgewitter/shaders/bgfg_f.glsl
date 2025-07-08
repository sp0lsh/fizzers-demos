#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    vec4 bg = texture2D(tex0, v2f_coord);
    vec4 fg = texture2D(tex1, v2f_coord);

    output_colour = bg * (1.0 - fg.a) + fg;
}

