#version 330

// fragment shader for blurring

uniform sampler2D tex;
uniform ivec2 axis;

noperspective in vec2 v2f_coord;
out vec4 f2a_col;

const float[9] kernel = float[9](0.1f, 0.2f, 0.5f, 0.7f, 1.0f, 0.7f, 0.5f, 0.2f, 0.1f);

float gauss(float a, float x)
{
    return sqrt(a / 3.1415926) * exp(-a * x * x);
}

void main()
{
    f2a_col = vec4(0.0);
    
    for(int i = -9; i <= +9; i += 1)
        //f2a_col += texelFetch(tex, ivec2(gl_FragCoord.xy) + ivec2(i) * axis, 0) * kernel[i];
        f2a_col += texelFetch(tex, ivec2(gl_FragCoord.xy) + ivec2(i) * axis, 0) * gauss(2.0, float(i) / 9.0);
    
    f2a_col *= 0.125;
}
