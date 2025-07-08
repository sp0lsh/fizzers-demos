#version 330

uniform sampler2D tex0;

out vec4 output_colour;

in vec3 v2f_normal;
in vec3 v2f_e;
in vec3 v2f_colour;

void main()
{
    vec3 n = normalize(v2f_normal);
    
    if(dot(n, v2f_e) > 0.0)
        n = -n;
    
    vec3 r=normalize(reflect(v2f_e,n));
    
    output_colour.rgb = vec3(0.5 + 0.5 * dot(n, normalize(vec3(-1.0, -1.5, 0.4)))) * v2f_colour;
    output_colour.rgb += vec3(pow(max(0.0,dot(r,normalize(vec3(0.4,1.0,-1.0)))),2.0))*0.75;
    output_colour.rgb += vec3(pow(max(0.0,dot(r,normalize(vec3(0.4,-1.0,1.0)))),2.0))*0.75;
    output_colour.a = 1.0;
}
