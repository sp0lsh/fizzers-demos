#version 400

// fragment shader for downsampling

in vec2 a2v_coord;
noperspective out vec2 v2f_coord;

void main()
{
    v2f_coord = a2v_coord;
    gl_Position = vec4(a2v_coord * 2.0 - vec2(1.0), 0, 1);
}

