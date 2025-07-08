#version 330

uniform sampler2D tex0;
uniform vec2 direction;

in vec2 v2f_coord;

out vec4 output_colour;

const int samp_count = 256;

float gauss(float a, float x)
{
    return sqrt(a / 3.1415926) * exp(-a * x * x);
}

void main()
{
    output_colour = vec4(0.0);

    for(int i = 0; i < samp_count; ++i)
    {
        float x = float(i) / float(samp_count) * 2.0 - 1.0;
        output_colour += texture2D(tex0, v2f_coord + direction * x) * gauss(2.0, x) * 1.0;
    }
    
    output_colour /= float(samp_count);
}

