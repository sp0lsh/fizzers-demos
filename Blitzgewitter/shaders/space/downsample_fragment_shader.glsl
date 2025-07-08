#version 400

// fragment shader for downsampling

uniform sampler2D tex;

noperspective in vec2 v2f_coord;
out vec4 f2a_col;

void main()
{
    f2a_col = vec4( dot(textureGather(tex, v2f_coord, 0), vec4(1.0 / 4.0)),
                    dot(textureGather(tex, v2f_coord, 1), vec4(1.0 / 4.0)),
                    dot(textureGather(tex, v2f_coord, 2), vec4(1.0 / 4.0)),
                    dot(textureGather(tex, v2f_coord, 3), vec4(1.0 / 4.0)) );
}
