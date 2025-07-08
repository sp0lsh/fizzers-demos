#version 330

uniform sampler2D tex0;
uniform float time;

in vec3 v2f_vo;
in vec3 v2f_o;
in float v2f_tetdist;

out vec4 output_colour0;

float fbm(vec2 p)
{
    float f=0.0;
    for(int i=0;i<4;++i)
    {
        f+=texture2D(tex0,p*exp2(i)).r/exp2(i+1);
    }
    return f;
}

float height(vec2 p)
{
    return (fbm(p*0.4+vec2(time*0.1*0.5,0.0))+fbm(p*0.3+vec2(time*0.05*0.5,0.0)))*0.02;
}

void main()
{
/*
    vec3 v=normalize(v2f_vo-v2f_o);
    vec3 l=normalize(vec3(0.0,0.9,0.0)-v2f_o);
    vec2 p=v2f_o.xz;
    float eps=1e-3;
    float h0=height(p);
    float h1=height(p+vec2(eps,0.0));
    float h2=height(p+vec2(0.0,eps));

    vec3 n=-normalize(cross(vec3(eps,h1-h0,0.0),vec3(0.0,h2-h0,eps)));
    
    output_colour0.a=1.0;
    output_colour0.rgb=vec3(0.1,0.1,0.2)*mix(0.2,0.0,smoothstep(0.85,1.25,length(v2f_o.xz)))*0.3;
    float f=sqrt(texture2D(tex0,p*0.5).g*0.5+texture2D(tex0,p).g*0.25)*1.5;
    output_colour0.rgb+=vec3(smoothstep(0.0,0.1,pow(clamp(1.0-smoothstep(0.015,0.04,v2f_tetdist),0.0,1.0),2.0)-f))*0.05;
    output_colour0.rgb*=max(0.0,dot(n,l));
    */
    output_colour0.a=1.0;
    output_colour0.rgb=vec3(0.1,0.1,0.2)*mix(0.2,0.0,smoothstep(0.85,1.25,length(v2f_o.xz)))*0.3;
}
