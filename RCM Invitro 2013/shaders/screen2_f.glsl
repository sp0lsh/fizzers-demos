#version 330

#define saturate(x) clamp(x, 0.0, 1.0)

uniform sampler2D display;
uniform sampler1D pal;
uniform sampler2D noise_tex;
uniform float aspect_ratio, time, tint;
uniform float dilate;
uniform float disappear;

noperspective in vec2 v2f_coord;

const vec2 resolution = vec2(80.0, 60.0);

out vec4 output_colour;

float mask(vec2 p)
{
   return 1.0 - smoothstep(0.05, 0.17, length(max(vec2(0.0), abs(p * vec2(0.75, 1.0)) - vec2(1.2))));
}

vec3 screen(vec2 p)
{
    vec2 f = fract(p);

    float m = saturate(pow(1.4 - length(f - vec2(0.5, 0.5)), 5.0));
    
    p = floor(p) / resolution;

    return texture(pal, texture2D(display, p).r).rgb * m * 0.5;
}

vec3 starfield(vec2 p)
{
    vec2 op=p;
    p+=vec2(time*0.3,0.0);
    vec2 j=floor(p);
    p=fract(p);
    vec3 c=vec3(0.0);
    float fb = smoothstep(-0.001, 0.0, time - 3.354);
    float dilate2 = mix(-0.1, dilate + disappear * 8.0, fb);
    for(int i=0;i<8;++i)
    {
        vec2 sp=vec2(0.5)+vec2(cos(i+j.y),sin(i*3+j.x))*0.44;
        float d=length((p-sp)*vec2(aspect_ratio,1.0));
        c += pow((1.0-smoothstep(0.001 + 0.006 * dilate2,0.006 + 0.01 * dilate2,d))*(0.5+0.5*cos((time+p.x+p.y*4.0)*2.0)),4.0) * (0.7+0.3*smoothstep(0.01,0.012,d));
    }
    return c*vec3(0.8,1.0,1.0)*(2.0+(1.0 - fb));
}

void main()
{
    vec3 p = vec3((v2f_coord.xy * 2.0 - vec2(1.0, 1.0)), 1.0);

    p.y *= 0.8;
    
    p *= pow(length(p), 1.2) * 0.7;
    //p *= pow(length(p), 0.3) * 0.8;

    float m=pow(clamp(1.0-dot(abs(p)*abs(p),abs(p)*abs(p))*0.8,0.0,1.0),0.5);

    vec3 op=p;
    
    p.xy = (p.xy + vec2(1.0, 1.0)) * 0.5;     
    
    float ds=mix(0.8, 4.0, disappear);//(1.0-dilate*0.2);
    
    output_colour.rgb = m*(vec3(0.02)+texture2D(display, p.xy).rgb * mix(vec3(1.0), vec3(0.7), step(0.8, fract(time * 2.0 - p.y * 2.0))))*1.0;
    output_colour.rgb += starfield(op.xy*ds);
    output_colour.rgb += starfield(op.xy*2.0*ds+vec2(100.0));
    output_colour.rgb += starfield(op.xy*4.0*ds+vec2(120.0));
    
    output_colour.rgb *= smoothstep(0.0, 3.0, time);
    output_colour.rgb += vec3(pow(disappear, 5.6));
    output_colour.rgb *= mix(vec3(1.0), vec3(1.0, 0.8, 1.0) * 0.5, smoothstep(0.0, 0.5, length((v2f_coord - vec2(0.5)) * vec2(aspect_ratio, 1.0))));
}
