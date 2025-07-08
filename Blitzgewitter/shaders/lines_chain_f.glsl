#version 330

uniform vec4 colour;

noperspective in vec3 lcoord;

out vec4 output_colour;

void main()
{
    if(length(lcoord.xy) > 1.0)
        discard;
  
    output_colour = colour;
}
