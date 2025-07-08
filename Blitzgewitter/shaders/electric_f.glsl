#version 330

uniform sampler2D tex0;
uniform float time;

noperspective in vec2 v2f_coord;

out vec4 output_colour0;

void main()
{
    float t = time*0.09;
    float a = texture2D(tex0,v2f_coord).r;
    output_colour0.r = smoothstep(0.0, 0.01, t - a) - smoothstep(0.01, 0.05, t - a);
}
