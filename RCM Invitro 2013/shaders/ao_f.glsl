#version 330

uniform sampler2D depth_tex;
uniform sampler2D noise_tex;
uniform vec4 ao_settings;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

vec2 ao_kernel[16] = vec2[16] (
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
 vec2(0.062500, 0.851852) );

 float ambientOcclusion()
{
    float d1 = texture2D(depth_tex, v2f_coord).r, ao = 0.0;

    float a = texelFetch(noise_tex, ivec2(mod(gl_FragCoord.xy, textureSize(noise_tex, 0))), 0).r * 2.0 * 3.1415926;

    mat2 rot = mat2(vec2(cos(a), sin(a)), vec2(sin(a), -cos(a)));
    
    for(int i = 0; i < ao_kernel.length(); ++i)
    {
        float d2 = texture2D(depth_tex, v2f_coord + rot * ao_kernel[i] * ao_settings.xy).r;
        ao += max(smoothstep(d1 - 0.3, d1, d2), smoothstep(0.5, 2.5, d1 - d2));
    }

    return ao / float(ao_kernel.length());
}
 
void main()
{
    output_colour.a = 1.0;
    output_colour.rgb = vec3(mix(1.0, pow(ambientOcclusion(), 0.8), ao_settings.z));
}

