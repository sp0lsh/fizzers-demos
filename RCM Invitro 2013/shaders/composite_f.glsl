#version 330

uniform sampler2D tex0, depth_tex;
uniform sampler2D tex1;
uniform vec4 tint;
uniform float darken;
uniform int frame_num;

noperspective in vec2 v2f_coord;

out vec4 output_colour;
 
void main()
{
    output_colour = texture2D(tex0, v2f_coord);// * mix(1.0, 1.0 - length(v2f_coord - vec2(0.5)), darken);
    output_colour *= tint;
    output_colour.rgb += output_colour.rgb * texelFetch(tex1, ivec2(mod(gl_FragCoord.xy + ivec2(frame_num * ivec2(23, 7)), textureSize(tex1, 0))), 0).rrr * 0.3;
    //output_colour.rgb += texelFetch(tex1, ivec2(mod(gl_FragCoord.xy + ivec2(frame_num * ivec2(23, 7)), textureSize(tex1, 0))), 0).rrr * 0.02;
    output_colour.rgb *= mix(vec3(0.2,0.5,1.0),vec3(0.9, 0.9, 1.0),pow(v2f_coord.x,0.5));
    output_colour = pow(max(vec4(0.0), output_colour), vec4(0.8));
}

