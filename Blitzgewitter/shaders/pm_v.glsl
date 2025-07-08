#version 330

in vec4 vertex;
out vec2 tc;

void main()
{
	tc = vertex.xy * 0.5 + vec2(0.5);
	gl_Position = vertex;
}
