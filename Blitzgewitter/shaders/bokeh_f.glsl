#version 330

uniform float aspect_ratio;

uniform sampler2D tex0;

uniform vec2 CoC;

noperspective in vec2 v2f_coord;

out vec4 output0, output1; // tex1, tex2

const int samp_count = 14;

void main()
{
    output0 = vec4(0.0);
    output1 = vec4(0.0);

    for(int i = 0; i < samp_count; ++i)
    {
        float x = (float(i) + 0.5) / float(samp_count);
        
        output0 += texture2D(tex0, v2f_coord + vec2( 0.0, -1.0) * CoC * x); // up
        output1 += texture2D(tex0, v2f_coord + vec2(+0.70711 * aspect_ratio, +0.70711) * CoC * x) * 1.4; // down-left
    }
    
    output0 /= float(samp_count);
    output1 /= float(samp_count);
    
    output1 += output0;
}
