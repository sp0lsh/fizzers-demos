#version 330

uniform sampler2D tex0;
uniform vec2 screen_res;

noperspective in vec2 v2f_coord;

out vec4 output_colour0;

void main()
{
    vec2 coord=v2f_coord+vec2(-0.25,0.0);
    vec2 sz=vec2(640.0,480.0);
    output_colour0.a = texture2D(tex0,coord*screen_res/sz*0.85).r;
    output_colour0.rgb = vec3(0.0);
}
