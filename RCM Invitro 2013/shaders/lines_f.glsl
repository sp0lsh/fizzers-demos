#version 330

uniform vec4 colour;
uniform float brightness;

uniform sampler2D logo_tex;
uniform vec2 logo_lcoord;
uniform float logo_bias;

noperspective in vec3 lcoord;

out vec4 output_colour;

void main()
{
    float a = 1.0 - smoothstep(0.1, 0.2, length(lcoord.xy));
    
    if(a < 0.1)
        discard;
    
    output_colour = a * colour * vec4(0.7, 0.8, 1.0, 1.0) * (0.7 + 0.7 * pow(abs(lcoord.z), 32.0))*0.8*brightness*mix(1.0, texture2D(logo_tex, logo_lcoord).r, logo_bias);
}
