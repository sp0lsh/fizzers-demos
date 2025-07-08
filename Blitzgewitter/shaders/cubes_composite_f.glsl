#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D bloom_tex, bloom2_tex;
uniform vec4 tint;
uniform int frame_num;
uniform float time;
uniform vec3 addcolour;
uniform vec3 addcolour2;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

float vignet()
{
    return 0.5 + 0.5*pow( 16*v2f_coord.x*v2f_coord.y*(1-v2f_coord.x)*(1-v2f_coord.y), 0.1 );
}


void main()
{
    output_colour = (texture2D(bloom2_tex, v2f_coord) * 2.0 + texture2D(bloom_tex, v2f_coord) * 1.) * 10.0;

    output_colour.rgb += output_colour.rgb * vec3(smoothstep(0.7, 1.0, texture2D(tex1, v2f_coord * 5.0 + vec2(0.0, time * 0.6)).r)) * 0.2;
    output_colour.rgb += output_colour.rgb * vec3(smoothstep(0.7, 1.0, texture2D(tex1, v2f_coord * 12.0 + vec2(0.0, time * 0.3)).r)) * 0.2;

    output_colour.rgb += texture2D(tex0, v2f_coord).rgb;
        
    output_colour.rgb += addcolour;
    output_colour.rgb = pow(output_colour.rgb, vec3(0.8)) * vignet() * tint.rgb +
        texelFetch(tex1, ivec2(mod(gl_FragCoord.xy + ivec2(frame_num * ivec2(23, 7)), textureSize(tex1, 0))), 0).rrr  / 255.0;
    output_colour.rgb += addcolour2;
}

