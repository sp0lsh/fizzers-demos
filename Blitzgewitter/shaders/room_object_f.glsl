#version 330

uniform vec3 material_colour;
uniform sampler3D noise_tex;

in vec3 g2f_n;
in vec3 g2f_v;
in vec3 g2f_o;

out vec4 output_colour0;

vec3 perturb(vec3 n)
{
    vec3 d=texture(noise_tex,g2f_o*8.0).rgb-vec3(0.5);
    return normalize(n+(d-dot(d,n)*n)*0.5);
}

void main()
{
    vec3 n=perturb(normalize(g2f_n));
    vec3 v=normalize(g2f_v);

    output_colour0.rgb=material_colour*0.2*(0.2+2.0*(pow(clamp(1.0+dot(n,v),0.0,1.0),1.7)));
    
    output_colour0.a=1.0;
}
