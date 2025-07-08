#version 330

uniform sampler3D tex0;

uniform float glow;
uniform float l0_brightness;

in vec3 v2f_n;
in vec4 w_pos;
in vec3 v2f_o;

noperspective in vec2 v2f_screen;
noperspective in vec2 v2f_zw;

out vec4 output_colour0;
out vec4 output_colour1;

vec3 perturb(vec3 n)
{
    vec3 d=texture(tex0,v2f_o*8.0).rgb-vec3(0.5)+(texture(tex0,v2f_o*4.0).rgb-vec3(0.5))*0.5;
    return normalize(n+(d-dot(d,n)*n)*0.5);
}

void main()
{
    vec3 n=perturb(normalize(v2f_n));
    vec3 col = vec3(smoothstep(0.93,0.935,length(v2f_o)))*vec3(0.5,0.55,1.0) * l0_brightness;
    output_colour0 = vec4(col, 1.0);
    output_colour0.rgb+=pow(clamp(0.5+0.5*n.y,0.0,1.0),4.0)*vec3(1.0,0.95,0.65)*glow;
    output_colour1 = vec4(w_pos.z / w_pos.w);
}
