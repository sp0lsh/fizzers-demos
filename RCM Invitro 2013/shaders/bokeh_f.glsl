#version 330

uniform float aspect_ratio;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

noperspective in vec2 v2f_coord;

out vec4 output0, output1; // tex1, tex2

const int samp_count = 32;

void main()
{
    float CoC = texture2D(tex0, v2f_coord).a;
    
    output0 = vec4(0.0);
    output1 = vec4(0.0);

    for(int i = 0; i < samp_count; ++i)
    {
        float x = (float(i) + 0.5) / float(samp_count);
        
        output0 += texture2D(tex0, v2f_coord + vec2( 0.0, -1.0) * CoC * x); // up
        output1 += texture2D(tex0, v2f_coord + vec2(+0.70711 * aspect_ratio, +0.70711) * CoC * x) * 1.4; // down-left
    }

    output1 += output0;
}


/*
float kernel(float x)
{
    return 1.0;//(1.0 - abs(x) * abs(x));
}

void main()
{
    output = vec4(0.0);
   
    float CoC = texture2D(tex1, v2f_coord).a;
   
    for(int i = 0; i <= samp_count; ++i)
    {
        float x = (float(i) / float(samp_count) - 0.5) * 2.0;
        output += texture2D(tex0, v2f_coord + direction * x * CoC) * kernel(x);
    }
    
   if(direction.x == 0.0)
        output /= output.a;
}
*/
