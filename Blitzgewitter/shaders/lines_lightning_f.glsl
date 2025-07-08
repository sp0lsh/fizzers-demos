#version 330

uniform vec4 colour;
uniform float time;

noperspective in vec3 lcoord;
noperspective in float zcoord;
in vec3 g2f_c;

out vec4 output_colour;

void main()
{
    float a = 1.0 - smoothstep(0.1, 0.4, length(lcoord.xy));
    
    if(a < 0.1)
        discard;

    gl_FragDepth = zcoord*0.5+0.5;
    output_colour = a * colour * vec4(g2f_c,1.0);
    output_colour.a = 0.0;
}
