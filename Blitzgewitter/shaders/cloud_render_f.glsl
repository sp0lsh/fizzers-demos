#version 330

uniform vec3 cam_origin;
uniform sampler3D tex0;
uniform sampler3D noise_tex;
uniform float time;
uniform float rayjitter;

out vec4 output_colour;

noperspective in vec2 v2f_coord;
noperspective in vec3 v2f_rd;

float cubic(float x)
{
   return (3.0 - 2.0 * x) * x * x;
}

float density(vec3 p)
{
    vec3 p2=p+vec3(time*0.0,0.0,0.0);
    float b=cubic(min(fract(time*0.2) * 2.0, (1.0 - fract(time*0.2)) * 2.0));
    return pow(1.0-smoothstep(0.0,0.5,distance(p, vec3(0.5))), mix(0.6,1.0,min(1.0,time*0.3))) *
            pow(clamp(dot(texture(noise_tex, p).rg, vec2(b, 1.0 - b)) + texture(noise_tex, p2.zxy * 2.0).r * 0.5 - 0.7, 0.0, 1.0), mix(0.05,1.0,min(2.0,time*0.2)));
}

void main()
{
    vec3 ro = cam_origin;
    vec3 rd = normalize(v2f_rd);
    
    float t=0.8+0.03 * rayjitter;
    float alpha=1.0;
    float scale=mix(2.0,0.75,time/(time+1.0))*0.75*0.5;
    vec3 glow=mix(2.1,0.0,time/(time+1.0))*vec3(1.0,1.0,0.8);
    vec3 col=vec3(0.0);
    for(int i=0;i<140;++i)
    {
        vec3 rp=ro+rd*t;
        float l=length(rp);
        rp=rp*scale+vec3(0.5);
        
        float d=density(rp);
        
        vec3 col2=vec3(texture(tex0,rp).r) + glow*(1.0-smoothstep(0.0,0.9,l));
        
        col=mix(col,col2,d*alpha);
        alpha*=1.0-d;
        
        t+=0.03;
    }
    
    output_colour.a = exp(-alpha);
    output_colour.rgb = pow(col,vec3(2.0));
}

