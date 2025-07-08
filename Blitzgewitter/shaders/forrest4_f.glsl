#version 330

uniform float time;
uniform float clip_side;

in vec2 v2f_coord;

in vec4 w_pos;

in vec3 v2f_v, v2f_n;
in vec3 v2f_o;

noperspective in vec2 v2f_screen;
noperspective in vec2 v2f_zw;

out vec4 output_colour0;
out vec4 output_colour1;

void main()
{
    if(v2f_o.x*clip_side < 0.0)
        discard;

    output_colour0 = vec4(vec3(0.0), 1.0);
}
