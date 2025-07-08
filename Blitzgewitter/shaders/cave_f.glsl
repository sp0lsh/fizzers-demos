#version 330

uniform sampler2D tex0;
uniform sampler3D tex1;
uniform float time;

in vec3 v2f_n;
in vec3 v2f_o;
in vec2 v2f_coord;

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

vec3 perturb(vec3 n,vec3 p)
{
    vec3 nn=texture(tex1,p*64.0).rgb-vec3(0.5);
    return n+normalize(nn-n*dot(nn,n))*0.6;
}

void main()
{
    vec3 n=normalize(perturb(normalize(v2f_n),v2f_o));
    vec3 l=normalize(vec3(0.0,1.1,0.0)-v2f_o);
    vec3 diff=vec3(1.0)*mix(0.5,1.0,texture2D(tex0,v2f_coord*vec2(4.0,2.0)*1.0).r);
    float f=fbm(v2f_coord);
    diff*=mix(vec3(0.5),vec3(1.0,0.9,0.8),smoothstep(0.5,0.6,f));
    output_colour0.a=1.0;
    output_colour0.rgb=diff*mix(vec3(0.0),vec3(0.8,0.82,1.0)*pow(1.0-v2f_o.y,3.0),max(0.0,0.25+0.75*n.y))*0.1*pow(0.5+0.5*cos(v2f_o.x*10.0+time*3.0)+0.5*(0.5+0.5*cos(cos(v2f_o.y*2.0+time*2.0)+v2f_o.z*15.0+time*4.0)),3.0);
    output_colour0.rgb+=diff*mix(vec3(0.0),vec3(1.0,1.0,0.9),max(0.0,-dot(n,l)))*0.05;
    output_colour0.rgb*=3.0;
    output_colour0.rgb*=sqrt(mix(1.0,0.0,smoothstep(1.0,1.2,length(v2f_o.xz))));
    output_colour0.rgb=mix(output_colour0.rgb,vec3(0.0),pow(clamp(-v2f_o.y,0.0,1.0),0.1));
}
