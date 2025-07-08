#version 330

// fragment shader for displaying everything

#define EPS 1e-2

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

void main()
{
    //vec4 overlay = texture(overlay_texture, v2f_coord);
    
    vec3 bl = softLight(texture(blur_texture, v2f_coord * 0.125).rgb * 0.5, vec3(219.0 / 255.0, 52.0 / 255.0, 164.0 / 255.0));
    
    f2a_col.rgb = softLight(texture(particle_output_texture, v2f_coord).rgb, vec3(219.0 / 255.0, 52.0 / 255.0, 164.0 / 255.0));
    f2a_col.a = 1.0;
    
    f2a_col.rgb = mix(f2a_col.rgb, bl, clamp(length((v2f_coord - vec2(0.5)) * vec2(16.0 / 9.0, 1.0)) * 1.5, 0.0, 1.0)) * 4.0;

    //f2a_col.rgb = mix(f2a_col.rgb * 4.0, overlay.rgb, overlay.a * circleWipe(smoothstep(5.0, 6.0, time)) * smoothstep(15.0, 10.0, time) );
}