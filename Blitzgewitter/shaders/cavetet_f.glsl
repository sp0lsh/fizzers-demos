#version 330

in vec2 v2f_coord;

in vec3 v2f_n;
in vec3 v2f_o;

uniform sampler2D tex0;

out vec4 output_colour;

vec2 texcoordForNormal(vec3 p,vec3 n)
{
    p-=vec3(0.000000, 0.666667, 0.235702);
    vec3 u=normalize(cross(n,vec3(0.0,-1.0,0.0)));
    vec3 v=normalize(cross(n,u));
    u=normalize(cross(v,n));
    return vec2(dot(p,u),dot(p,v))/vec2(1.6,1.42)+vec2(0.5,1.0);
}

void main()
{
   output_colour.a=1.0;
   output_colour.rgb=texture2D(tex0,texcoordForNormal(v2f_o,v2f_n)).rgb*0.5;
}

