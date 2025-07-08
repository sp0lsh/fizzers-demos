#version 330

// fragment shader for displaying everything

uniform sampler2D particle_output_texture;
uniform sampler2D overlay_texture;
uniform sampler2D blur_texture;
uniform float time;

noperspective in vec2 v2f_coord;
out vec4 f2a_col;

float brightness(vec3 c)
{
   return dot(c, vec3(1.0 / 3.0));
}

vec3 softLight(vec3 t, vec3 b)
{
   return mix(vec3(1.0) - (vec3(1.0) - t) * (vec3(1.0) - (b - vec3(0.5))),
               t * (b + vec3(0.5)), step(brightness(t), 0.5));
}

float circleWipe(float t)
{
    return smoothstep(0.0, -0.01, length((v2f_coord - vec2(0.5)) * vec2(16.0 / 9.0, 1.0)) - (t * 2.0));
}

vec3 dither()
{
    return vec3(fract(gl_FragCoord.x * 0.564636 + gl_FragCoord.y * 0.912489) * (1.0 / 255.0));
}

void main()
{
    vec3 bl = softLight(texture(blur_texture, v2f_coord * 0.125).rgb * 0.5, vec3(1.0,1.0,0.8))*6.0;
    
    f2a_col.rgb = 0.02*-pow(softLight(texture(particle_output_texture, (v2f_coord-0.5)*0.8+vec2(0.5)).rgb, vec3(1.0,1.0,0.9))*2.0,vec3(0.4));
    f2a_col.rgb += softLight(texture(particle_output_texture, v2f_coord).rgb, vec3(1.0,1.0,0.9))*2.0;
    
    f2a_col.a = 1.0;
    
    f2a_col.rgb = mix(f2a_col.rgb, bl, 0.75 * clamp(length((v2f_coord - vec2(0.5)) * vec2(16.0 / 9.0, 1.0)), 0.0, 1.0)) * 4.0;
    
    f2a_col.rgb+=2.5*vec3(1.0-smoothstep(0.0,0.8,length((v2f_coord - vec2(0.5)) * vec2(16.0 / 9.0, 1.0))))*0.027*vec3(1.0,1.0,1.0)*mix(0.95,1.0,0.5+0.5*(sin(time*16.0)+cos(time*14.0)));

    f2a_col.rgb*=vec3(1.08,1.05,1.0);
    
    f2a_col.rgb += dither();
}