#version 330

uniform mat4 modelview_inv;
uniform sampler3D tex0;
uniform sampler3D noise_tex;
uniform sampler2D noise_tex_2d;
uniform float time;

out vec4 output_colour;

noperspective in vec2 v2f_coord;
noperspective in vec3 v2f_rd;

float cubic(float x)
{
   return (3.0 - 2.0 * x) * x * x;
}

float density(vec3 p)
{
    p.z/=2.0;
    return pow(texture(noise_tex, p*2.0).r*0.75+texture(noise_tex, p*4.0).r*0.25,2.0)*smoothstep(0.0,0.02,p.z);
}


void main()
{
    vec3 ro = vec3(0.0);//(modelview_inv * vec4(0.0,0.0,0.0,0.0)).xyz;
    ro.z+=0.9+max(0.0,(time-15.0)*0.04);
    ro.xy=vec2(0.0);
    vec3 rd = normalize(v2f_rd);
    
    ro.x+=time*0.5;
    ro.y+=time*0.2;
    
    ro.z-=2.0;
    
    float t=0.1 * texelFetch(noise_tex_2d,ivec2(gl_FragCoord.xy)&127,0).r;
    float alpha=1.0;
    vec3 col=vec3(0.0);
    ro=ro*0.5+vec3(0.5);
    rd=rd*0.5;
    rd.z*=3.0;
    ro.z*=3.0;
    for(int i=0;i<30;++i)
    {
        vec3 p=ro+rd*t;
        
        //p.z*=3.0;
        float d=density(p)*(1.0+(texture(noise_tex, p*8.0).g-0.5)*2.0*0.2);
        
        p.z=1.0-p.z;
        vec3 col2=vec3(texture(tex0,p).r)*vec3(3.0)*(vec3(1.0)+2.0*vec3(0.7,0.8,1.0)*max(0.0,cos(p.x*10.0)*sin(p.y*10.0+sin(p.x)*10.0)));
        //vec3 col2=vec3(texture(tex0,p).r)*vec3(4.0);
        
        col=mix(col,col2,d*alpha);
        alpha*=1.0-d;
        
        t+=0.1;
    }
    
    output_colour.a = exp(-alpha);
    output_colour.rgb = mix(mix(vec3(0.0),vec3(0.075,0.075,0.1),0.5+0.5*rd.z),pow(col,vec3(1.2)),output_colour.a);
}

