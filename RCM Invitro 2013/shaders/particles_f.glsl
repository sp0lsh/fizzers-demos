#version 330

uniform vec4 colour;
uniform float brightness;
uniform float disappear;

uniform sampler2D logo_tex;
uniform float logo_bias;

in GeometryOut
{
    noperspective vec2 tc;
    noperspective vec2 coord;

} g_out;

out vec4 output_colour;

void main()
{
    float a = 1.0 - smoothstep(0.01, 0.2 + disappear * 0.8, length(g_out.coord));
    
    if(a < 0.1)
        discard;
    
    output_colour = a * colour * vec4(0.7, 0.8, 1.0, 1.0) * brightness * mix(1.0, texture2D(logo_tex, g_out.tc).r, logo_bias) * pow(1.0 - disappear, 3.0);
}
