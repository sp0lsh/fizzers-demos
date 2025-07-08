#version 330

uniform float aspect_ratio;

uniform sampler2D tex1;
uniform sampler2D tex2;

uniform vec2 CoC;
uniform float brightness;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

const int samp_count = 14;

void main()
{
    output_colour = vec4(0.0);

    for(int i = 0; i < samp_count; ++i)
    {
        float x = (float(i) + 0.5) / float(samp_count);
        
        output_colour += texture2D(tex2, v2f_coord + vec2(-0.70711 * aspect_ratio, +0.70711) * CoC * x); // down-right
        output_colour += texture2D(tex1, v2f_coord + vec2(+0.70711 * aspect_ratio, +0.70711) * CoC * x); // down-left
    }

    output_colour /= float(samp_count) / brightness;
}
