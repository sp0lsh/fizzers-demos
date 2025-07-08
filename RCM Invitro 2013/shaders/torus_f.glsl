#version 330

uniform float time;
uniform float arp;

in VertexOut
{
    noperspective vec3 opos;
    noperspective vec3 vpos;

} v_out;

out vec4 output_colour;

float quant(float x)
{
    return floor(x * 16.0) / 16.0;
}

float chequerboard(vec2 p)
{
    return step(0.5, fract(p.x + 0.5 * step(0.5, fract(p.y))));
}

float pyramid(float x)
{
   x = fract(x);
   return clamp(min(-1.0 + x * 4.0,  +3.0 - x * 4.0), -0.5, +0.5);
}

vec3 tex(vec2 p)
{
    vec3 c = vec3(0.0);
    
    for(int i = 0; i < 16; ++i)
    {        
        float a = pyramid(p.y * 0.2 + float(i) * 0.3) * 1.0;
        float x = p.x + (0.5 + 0.5 * cos(float(i))) * 16.0;
        c += vec3((smoothstep(a - 0.01, a, x) - smoothstep(a + 0.49, a + 0.5, x)) * vec3(0.3, 1.0, 0.3)) * (1.0 + pow(0.5 + 0.5 * cos(p.y), 20.0) * 10.0) * 0.1;
    }
    
    for(int i = 0; i < 16; ++i)
    {        
        float a = pyramid(p.y * 0.2 + float(i+100) * 0.3) * 1.0;
        float x = p.x + (0.5 + 0.5 * cos(float(i+100))) * 16.0;
        c += vec3((smoothstep(a - 0.01, a, x) - smoothstep(a + 0.05, a + 0.06, x)) * vec3(0.3, 1.0, 0.3)) * (1.0 + pow(0.5 + 0.5 * cos(p.y+10.0), 20.0) * 10.0) * 0.1;
    }
    
    return (vec3(0.0, 0.02, 0.0) + c) * 0.3;
}

void main()
{
    vec3 vertex = v_out.opos;
    vec3 mid = vec3(normalize(vertex.xz), 0.0).xzy;
    vec2 tc = vec2(atan(vertex.y, dot(vertex.xyz - mid, mid)) / 3.1415926 - 1.0, atan(vertex.z, vertex.x) / 3.1415926);

//    output_colour = vec4(tex(tc * 10.0), 1.0) * quant(mix(0.2, 1.0, smoothstep(-30.0 * 8, -2.0 * 4, v_out.vpos.z))) * 0.5;

    float yy = floor(tc.y * 64.0 + cos(tc.x * 10.0 * 3.1415926)) / 64.0;

    output_colour.rgb = vec3(0.5 + 0.5 * cos(yy * 10.0),0.5 + 0.5 * cos(yy * 16.0),0.5 + 0.5 * cos(yy * 20.0)) * 0.8;
    
    float yy2 = floor((tc.y * 6.0 + time * 4.0) * 16.0) / 16.0;
    
    output_colour.rgb = mix(output_colour.rgb, output_colour.rgb * (0.6f + mix(vec3(0.7), vec3(0.5 + 0.5 * cos(yy2 * 5.0),0.5 + 0.5 * cos(yy2 * 12.0),0.5 + 0.5 * cos(yy2 * 11.0)).bgr, 0.5)), arp);
}
