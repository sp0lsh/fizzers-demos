#version 330

uniform float time;
uniform sampler2D noise_tex;
uniform vec3 colour;
uniform float fog;
uniform float flash;

in vec2 v2f_coord;

in vec3 v2f_v, v2f_n;
in vec3 v2f_o;

noperspective in vec2 v2f_screen;

out vec4 output_colour0;

void main()
{
    vec3 col=mix(vec3(1.0,0.8,0.5),vec3(0.5,0.7,1.0),texture2D(noise_tex,vec2(time*0.64,0.0)).r * 0.2 + 0.4+0.4*cos(time*3.4)*sin(time*5.0))*0.5*mix(0.9,1.0,sin(time*18.0)*0.5+0.5)*texture2D(noise_tex,vec2(time*0.2,0.0)).r;

    col*=mix(0.6,1.0,smoothstep(0.3,0.31,texture2D(noise_tex,v2f_screen.xy*0.2*vec2(1.0,0.2)+vec2(0.0,time*0.3)).r));
    col=mix(vec3(0.1),col,flash);
    
    output_colour0 = vec4(mix(vec3(0.06,0.07,0.1)*0.2,mix(vec3(0.6),col,smoothstep(0.0,1.0,time))*colour,exp(v2f_v.z*0.01)), 1.0);
}
