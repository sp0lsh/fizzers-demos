#version 330

#define saturate(x) clamp(x, 0.0, 1.0)

uniform sampler2D reveal;
uniform sampler1D pal;
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

void main()
{
    vec3 p = vec3((v2f_coord.xy * 2.0 - vec2(1.0, 1.0)) * vec2(aspect_ratio, 1.0), 1.0);

    p *= pow(length(p), 1.2);

    float m = mask(p.xy);
    
    p.xy = (p.xy + vec2(1.0, 1.0)) * 0.5 * resolution;
    
    output_colour.rgb = screen(p.xy + vec2( 0.4, 0.0)) * m * vec3(0.9, 0.1, 0.1) +
                 screen(p.xy + vec2( 0.0, 0.0)) * m * vec3(0.1, 0.9, 0.1) +
                 screen(p.xy + vec2(-0.4, 0.0)) * m * vec3(0.1, 0.1, 0.9);
                 
    output_colour = vec4(0.0);
}

