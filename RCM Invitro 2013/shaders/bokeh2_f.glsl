#version 330

uniform float aspect_ratio;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

const int samp_count = 32;

void main()
{
    float CoC = texture2D(tex0, v2f_coord).a;
    
    output_colour = vec4(0.0);

    for(int i = 0; i < samp_count; ++i)
    {
        float x = (float(i) + 0.5) / float(samp_count);
        
        output_colour += texture2D(tex2, v2f_coord + vec2(-0.70711 * aspect_ratio, +0.70711) * CoC * x); // down-right
        output_colour += texture2D(tex1, v2f_coord + vec2(+0.70711 * aspect_ratio, +0.70711) * CoC * x); // down-left
    }

    output_colour /= output_colour.a;
}


/*
float kernel(float x)
{
    return 1.0;//(1.0 - abs(x) * abs(x));
}

void main()
{
    output_colour = vec4(0.0);
   
    float CoC = texture2D(tex1, v2f_coord).a;
   
    for(int i = 0; i <= samp_count; ++i)
    {
        float x = (float(i) / float(samp_count) - 0.5) * 2.0;
        output_colour += texture2D(tex0, v2f_coord + direction * x * CoC) * kernel(x);
    }
    
   if(direction.x == 0.0)
        output_colour /= output_colour.a;
}
*/
