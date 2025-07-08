#version 330

#define saturate(x) clamp(x, 0.0, 1.0)
#define EPS vec3(0.001, 0.0, 0.0)

uniform vec2 CoCScaleAndBias;
uniform sampler2D colour_tex;
uniform sampler3D noiseTex;
uniform float time, material, bump_scale;
uniform vec3 colour;

in vec2 v2f_coord;

in vec3 v2f_v, v2f_n, v2f_l1, v2f_l2, v2f_l3;
in vec3 v2f_o;

noperspective in vec2 v2f_screen;
noperspective in vec2 v2f_zw;

out vec4 output_colour0;
out vec4 output_colour1;

vec3 phong(vec3 diff, vec3 spec, vec3 l, vec3 n, vec3 h)
{
    return saturate(dot(l, n)) * diff + pow(saturate(dot(n, h)), 80.0) * spec * 7.0;
}

vec3 phong2(vec3 diff, vec3 spec, vec3 l, vec3 n, vec3 h)
{
    // wrap lighting
    return saturate(dot(l, n) * 0.75 + 0.25) * diff + pow(saturate(dot(n, h)), 80.0) * spec * 7.0;
}

vec3 perturb(vec3 n, vec3 v)
{
    return normalize(n + (v - n * dot(v, n)) * 0.004);
}

vec3 noise3(vec3 p)
{
    return texture(noiseTex, p).xyz * 2.0 - 1.0;
}

vec3 noiseGradient(vec3 p)
{
    return normalize(noise3(p * 0.08) + noise3(p * 0.16) * 0.5);
}

void main()
{
    // for bokeh depth-of-field
    float CoC = max(abs(v2f_zw.x / v2f_zw.y * CoCScaleAndBias.x + CoCScaleAndBias.y), 0.001);

    vec3 l1 = normalize(v2f_l1), n = perturb(normalize(v2f_n), noiseGradient(v2f_o * 3.0) * bump_scale);
    vec3 v = normalize(v2f_v), l2 = normalize(v2f_l2), l3 = normalize(v2f_l3);

    vec3 col;
  
    vec3 colour2 = mix(colour, vec3(0.5), 0.5);
    
    col = vec3(0.3)*colour2*vec3(0.9,0.9,1.0) + vec3(1.2,1.2,1.0) * phong2(vec3(0.3)*colour2, vec3(0.8), l2, n, normalize(l2 + -v));
    col *= 1.0 - smoothstep(0.0, 200.0, length(v2f_v));
    
    if(material > 0.5)
       col = mix(vec3(1.0, 1.0, 0.8), vec3(0.95, 0.95, 1.0)*0.5, cos(v.y * 10.0+time) * 0.5 + 0.5) * mix(8.0, 1.0, smoothstep(0.0, 12.0, time));
    
    col *= mix(1.0, pow(max(0.0, 1.0 - length(v2f_screen * v2f_screen * v2f_screen)), 2.0), 0.7) * 0.7;

    output_colour0 = vec4(col * CoC, CoC);
    output_colour1 = -v2f_v.zzzz; // for ambient occlusion

    if(material > 0.5)
        output_colour1 = vec4(1e2);
}
