#version 330

uniform vec4 colour;
uniform sampler2D tex0;
uniform float tbias;
uniform float brightness;

in VertexOut
{
    noperspective vec2 tc;
    noperspective vec3 vpos;

} v_out;

out vec4 output_colour;

float quant(float x)
{
    return floor(x * 16.0) / 16.0;
}

void main()
{
    output_colour = brightness * mix(colour, texture2D(tex0, v_out.tc) * colour, tbias) * quant(mix(0.2, 1.0, smoothstep(-30.0, -8.0, v_out.vpos.z)));
}
