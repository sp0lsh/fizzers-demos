#version 330

uniform vec4 colour;
uniform float brightness;
uniform float time;

noperspective in vec3 lcoord;

out vec4 output_colour;

void main()
{
    float a = 1.0 - smoothstep(0.1, 0.4, length(lcoord.xy));
    
    if(a < 0.1)
        discard;
    
    output_colour = a * colour * brightness;
    output_colour.a = 0.0;
}
