#version 330

uniform sampler2D tex0;
uniform float time;

in vec3 v2f_o;
in vec2 v2f_c;

out vec4 output_colour0;

void main()
{
    vec4 s=texture2D(tex0, v2f_c);
    output_colour0.rgb=mix(vec3(0.5),s.rgb*0.3,pow(min(1.0,time*0.07),0.5));
    output_colour0.a=s.a;
}
