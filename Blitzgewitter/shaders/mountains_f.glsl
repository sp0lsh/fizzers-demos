#version 330

uniform vec3 colour;
uniform sampler3D tex0;

in vec3 v2f_v;
in vec3 v2f_o;

out vec4 output_colour0;

void main()
{
    float f=(texture(tex0,v2f_o.xyz*2.0).r-0.5)*0.5;
    output_colour0.a=1.0;
    output_colour0.rgb=mix(colour,vec3(0.0),exp(v2f_v.z*v2f_v.z*-0.0005+f));
}
