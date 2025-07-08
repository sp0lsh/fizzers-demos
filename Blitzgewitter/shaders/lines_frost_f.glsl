#version 330

uniform vec4 colour;
uniform float brightness;
uniform float time;

noperspective in vec3 lcoord;
noperspective in float zcoord;

out vec4 output_colour;

void main()
{
    float a = 1.0 - smoothstep(0.1, 0.4, length(lcoord.xy));
    
    if(a < 0.1)
        discard;

    gl_FragDepth = zcoord*0.5+0.5;
    output_colour = a * colour * brightness;
    output_colour.a = 0.0;
}
