#version 330

uniform float time;
uniform float ptn_thickness;
uniform float ptn_brightness;
uniform float ptn_scroll;
uniform float ptn_frequency;
uniform float shimmer;
uniform float l0_scroll;
uniform float l0_brightness;
uniform float l1_scroll;
uniform float l1_brightness;
uniform float l2_scroll;
uniform float l2_brightness;
uniform float ambient;

in vec2 v2f_coord;

in vec3 v2f_v, v2f_n;
in vec3 v2f_o;

out vec4 output_colour0;
out vec4 output_colour1;

float df(vec3 p)
{
   vec3 ap=abs(p);
   float d0=max(max(ap.x,ap.y),(abs(ap.x)+abs(ap.y))*0.7171);
   float d1=max(max(ap.y,ap.z),(abs(ap.y)+abs(ap.z))*0.7171);
   float d2=max(max(ap.z,ap.x),(abs(ap.z)+abs(ap.x))*0.7171);

   return min(d0,min(d1,d2));
}

vec3 ptn(vec3 p)
{
    return mix(vec3(0.0), vec3(1.0), step(fract(time*ptn_scroll+df(p)),ptn_thickness));
}

void main()
{
    vec3 col = vec3(1.0-smoothstep(0.001, 0.002, abs(fract(length(v2f_o)-time*l0_scroll) - 0.5)) )*vec3(0.5,0.55,1.0) * l0_brightness;
    
    col += vec3(1.0-smoothstep(0.017, 0.02, abs(fract(length(v2f_o)-time*l1_scroll - 0.3) - 0.5)) )*vec3(1.0, 0.85, 0.6) * l1_brightness;
    col += vec3(1.0-smoothstep(0.025, 0.03, abs(fract(length(v2f_o)-time*l2_scroll) - 0.5)) )*vec3(0.5,0.55,1.0) * l2_brightness;
    col += vec3(0.8,1.0,1.0) * ambient;
    col *= mix(1.0,mix(0.8, 1.0, 0.5 + 0.5 * cos(time * 80.0))*0.6, shimmer);    
    col += ptn(v2f_o*ptn_frequency)*ptn_brightness;
    
    output_colour0 = vec4(col, 1.0);
}
