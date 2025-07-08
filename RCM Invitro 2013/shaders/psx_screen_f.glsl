#version 330

#define saturate(x) clamp(x, 0.0, 1.0)

uniform sampler2D reveal;
uniform sampler1D pal;
uniform sampler2D logo_tex;
uniform float aspect_ratio, time, tint;

noperspective in vec2 v2f_coord;

const vec2 resolution = vec2(32.0, 24.0);

out vec4 output_colour;

float mask(vec2 p)
{
   return smoothstep(0.08, 0.1, length(max(vec2(0.0), abs(p * vec2(0.75, 1.0)) - vec2(1.2))));
}

vec3 screen(vec2 p)
{
    vec2 f = fract(p);

    float m = saturate(pow(1.4 - length(f - vec2(0.5, 0.5)), 5.0));
    
    vec3 rc = vec3(step(max(0.0, 2.0 - time * 0.2) + (1.0 - tint), texelFetch(reveal, ivec2(mod(p, textureSize(reveal, 0))), 0).r));

    p = floor(p) / resolution;

    float c = mod(p.y + time * 0.2 + cos(p.x + time), 1.0);
    
    c = mix(c, step(0.5, mod(p.y * 0.4 - time, 1.0)) * 0.2, sin(time * 0.2));
    
    return texture(pal, c).rgb * m * rc;
}

vec2 rotate(float a, vec2 v)
{
    return vec2(cos(a) * v.x + sin(a) * v.y, cos(a) * v.y - sin(a) * v.x);
}

void main()
{
/*
    vec2 p = rotate(-0.4, (v2f_coord.xy * 2.0 - vec2(1.0, 1.0)) * vec2(aspect_ratio, 1.0)) * vec2(450.0 / 815.0, 1.0) * 3.0;

    vec2 res = vec2(aspect_ratio, 1.0) * 32.0;
    
    p = floor(p * res) / res;
    
    vec2 c = floor(p);
    vec2 f = fract(p);
    
    float beat = 0.9 + (1.0 - fract(time));
    
    output_colour.a = 1.0;
    output_colour.rgb = mix(vec3(0.1), vec3(0.0,0.0,0.02), 1.0 - texture2D(logo_tex, p + vec2(0.0, 0.7 * time * (-1.0 + 2.0 * mod(c.x, 2.0))) ).r) * 0.1 * mix(2.0, 0.4, pow(clamp(-p.x * 0.25, 0.0, 1.0), 0.2)) * beat;
    */
    
    output_colour = vec4(0.0);
}

