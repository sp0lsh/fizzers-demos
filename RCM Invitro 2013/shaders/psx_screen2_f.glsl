#version 330

#define saturate(x) clamp(x, 0.0, 1.0)

uniform sampler2D display;
uniform sampler2D credits_tex;
uniform sampler2D noise_tex;
uniform sampler2D gbtex;
uniform float aspect_ratio, time, tint;
uniform float dilate;
uniform float gb_appear;

noperspective in vec2 v2f_coord;

const vec2 resolution = vec2(80.0, 60.0);
const vec2 gbres = vec2(160.0, 144.0);

out vec4 output_colour;

float mask(vec2 p)
{
   return 1.0 - smoothstep(0.05, 0.17, length(max(vec2(0.0), abs(p * vec2(0.75, 1.0)) - vec2(1.2))));
}

float gbpix(vec2 p, float t)
{
    return max(texture2D(gbtex, (floor((p + vec2(0.0, clamp(1.0 - t * 0.2, 0.0, 1.0))) * gbres) + vec2(0.4)) / gbres ).r, step(mix(0.78, 1.0, step(21.0, time)), p.x));
}

float gb(vec2 p)
{
    float a=0.0,wsum=0.0;
    int n = 6;
    for(int i=0;i<n;++i)
    {
        float w = 1.0 / float(i + 1);
        a += gbpix(p, time - float(i) * 0.02 - 15.0) * w;
        wsum+=w;
    }
    return a/wsum;
}

float gbdot(vec2 p)
{
    float b = 0.2, s = 0.2;
    return mix(0.95, 1.0, min(smoothstep(0.0, s, p.x - b), smoothstep(0.0, s, p.y - b)));
}

vec3 greyish(vec3 c)
{
    return mix(c, vec3(dot(c, vec3(1.0 / 3.0))), 0.7);
}

void main()
{
    vec3 p = vec3((v2f_coord.xy * 2.0 - vec2(1.0, 1.0)), 1.0);
    vec2 pp = p.xy;

    vec2 resolution = vec2(aspect_ratio * 160.0, 160.0);
    
    p.xy = floor(p.xy * resolution) / resolution;

    float m=pow(clamp(1.0-dot(abs(p)*abs(p),abs(p)*abs(p))*0.8,0.0,1.0),0.5);

    vec3 op=p;
    
    p.xy = (p.xy + vec2(1.0, 1.0)) * 0.5;     
    
    float ds=1.0;//(1.0-dilate*0.2);
    
    output_colour.rgb = texture2D(display, p.xy).rgb*mix(0.95, 1.0, (0.5+0.5*cos(p.y*1280.0)));

    vec2 gbc = (v2f_coord - vec2(0.5)) * vec2(aspect_ratio, 1.0) * (4.4 - gb_appear);
    float gbm = smoothstep(0.0, 0.01, 1.0 - abs(gbc.x)) * smoothstep(0.0, 0.01, 1.0 - abs(gbc.y));
    
    gbc = gbc * 0.5 + vec2(0.5);
    
    output_colour.rgb = mix(output_colour.rgb * (1.0 - gb_appear), gbdot(fract(gbc * gbres)) * 2.0 * greyish(mix(vec3(0.05, 0.1, 0.01) * 0.5, vec3(0.08, 0.2, 0.02), gb(gbc))), gb_appear * gbm) * (1.0 - smoothstep(0.0, 3.5, time - 22.0));
    
    output_colour.rgb += texture2D(credits_tex, pp * vec2(19.0 / 188.0 * aspect_ratio, 1.0) * 9.0 + vec2(0.487, 7.5)).rgb * (smoothstep(22.0, 23.0, time) - smoothstep(25.0, 26.0, time));
}
