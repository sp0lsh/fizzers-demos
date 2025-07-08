#version 330

uniform sampler3D tex0;
uniform sampler3D noise_tex;
uniform int layer;
uniform float layerf;

out vec4 output_colour;

noperspective in vec2 v2f_coord;

float density(vec3 p)
{
    p.z/=2.0;
    return pow(texture(noise_tex, p*2.0).r*0.75+texture(noise_tex, p*4.0).r*0.25,2.0)*smoothstep(0.0,0.02,p.z);
}


void main()
{
    float x=0.26;
    if(layer == 0)
    {
        float f=density(vec3(v2f_coord, 1.0-layerf));
        output_colour.r=exp(-f*x);
    }
    else
    {
        float f=density(vec3(v2f_coord, 1.0-layerf));
        float a=texelFetch(tex0, ivec3(ivec2(gl_FragCoord.xy), layer - 1), 0).r;
        output_colour.r=a*exp(-f*x);
    }
}
