#version 330

uniform sampler2D tex0;

noperspective in vec2 v2f_coord;

out vec4 output_colour;


void main()
{
    vec2 d = 1.0 / textureSize(tex0, 0);
    output_colour = (texture2D(tex0, v2f_coord + d * vec2(0.0, 0.0)) + 
              texture2D(tex0, v2f_coord + d * vec2(1.0, 0.0)) +
              texture2D(tex0, v2f_coord + d * vec2(0.0, 1.0)) +
              texture2D(tex0, v2f_coord + d * vec2(1.0, 1.0))) * 0.25;

}

