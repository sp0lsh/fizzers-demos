#version 330

uniform sampler3D tex0;
uniform sampler3D noise_tex;
uniform int layer;
uniform float layerf;
uniform float time;

out vec4 output_colour;

noperspective in vec2 v2f_coord;

float cubic(float x)
{
   return (3.0 - 2.0 * x) * x * x;
}

float density(vec3 p)
{
    vec3 p2=p+vec3(time*0.0,0.0,0.0);
    float b=cubic(min(fract(time*0.2) * 2.0, (1.0 - fract(time*0.2)) * 2.0));
    return pow(1.0-smoothstep(0.0,0.5,distance(p, vec3(0.5))), mix(0.6,1.0,min(1.0,time*0.3))) * pow(clamp(dot(texture(noise_tex, p).rg, vec2(b, 1.0 - b)) + texture(noise_tex, p2.zxy * 2.0).r * 0.5 - 0.7, 0.0, 1.0), mix(0.05,1.0,min(2.0,time*0.2)));
}



void main()
{
    float x=2.0;
    if(layer == 0)
    {
        float f=density(vec3(v2f_coord, layerf));
        output_colour.r=exp(-f*x);
    }
    else
    {
        float f=density(vec3(v2f_coord, layerf));
        float a=texelFetch(tex0, ivec3(ivec2(gl_FragCoord.xy), layer - 1), 0).r;
        output_colour.r=a*exp(-f*x);
    }
}
