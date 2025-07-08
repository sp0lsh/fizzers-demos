#version 330

uniform sampler2D tex0;

noperspective in vec2 lcoord;

in vec2 bri_tofs;

out vec4 output_colour;

void main()
{
    if(length(lcoord.xy) > 0.8)
        discard;

    output_colour.r = bri_tofs.x*pow(1.0 - smoothstep(0.0,0.8,length(lcoord.xy)),2.0)*(0.2 + texture2D(tex0,lcoord*0.05+vec2(bri_tofs.y)).r*0.05);
}
