#version 330

uniform sampler2D tex0;
uniform sampler2D noise_tex;
uniform vec2 scale;

in vec2 v2f_coord;

out vec4 output_colour;

const int samp_count = 32;

void main()
{
    output_colour = vec4(0.0);

    vec2 direction = (v2f_coord - vec2(0.5, 0.5)) * scale;
    float weights_sum = 0.0f;
    
    float xofs = texelFetch(noise_tex, ivec2(mod(gl_FragCoord.xy + vec2(16.0), textureSize(noise_tex, 0))), 0).r;
    
    for(int i = 0; i < samp_count; ++i)
    {
        float x = (float(i) + xofs) / float(samp_count), weight = 1.0 - x;
        output_colour += texture2D(tex0, v2f_coord + direction * x) * weight;
        weights_sum += weight;
    }
    
    output_colour /= weights_sum;
}

