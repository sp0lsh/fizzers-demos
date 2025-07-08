#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform float time;
uniform float aspect_ratio;

in vec2 v2f_coord;

out vec4 output_colour0;

vec2 kernel[16] = vec2[16](
 vec2(0.000000, -0.333333),
 vec2(-0.500000, 0.333333),
 vec2(0.500000, -0.777778),
 vec2(-0.750000, -0.111111),
 vec2(0.250000, 0.555556),
 vec2(-0.250000, -0.555556),
 vec2(0.750000, 0.111111),
 vec2(0.125000, -0.925926),
 vec2(-0.375000, -0.259259),
 vec2(0.625000, 0.407407),
 vec2(-0.625000, -0.703704),
 vec2(0.375000, -0.037037),
 vec2(-0.125000, 0.629630),
 vec2(0.875000, -0.481481),
 vec2(-0.937500, 0.185185),
 vec2(0.062500, 0.851852)
);
      
void main()
{
    vec2 r=normalize(texelFetch(tex1,ivec2(gl_FragCoord.xy)%ivec2(128), 0).xy);
 
    float a=0.0;
    for(int i=0;i<16;++i)
    {
        a+=texture2D(tex0,v2f_coord+reflect(kernel[i],r)*0.0025).r;
    }
    
    output_colour0.rgb=vec3(a/16.0)*vec3(0.7,0.9,1.0)*max(0.0,mix(3.0,0.5,time*0.2));
}
