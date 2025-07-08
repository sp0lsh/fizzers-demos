#version 330

uniform sampler2D positions_tex;

out int id;

void main()
{
    id = gl_VertexID;
    int s = int(textureSize(positions_tex, 0).x);
    vec4 vert = texelFetch(positions_tex, ivec2(gl_VertexID % s, gl_VertexID / s), 0);
	gl_Position = vert;
}
