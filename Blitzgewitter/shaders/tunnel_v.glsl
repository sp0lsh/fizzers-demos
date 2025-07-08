#version 330

uniform mat4 projection, modelview;
uniform sampler3D tex0;

in vec4 vertex;
in vec3 normal;
in vec2 coord;

out vec3 v2f_v, v2f_n;
out vec3 v2f_o;

out float v2f_h;

float snoise(vec3 v)
{
    return texture(tex0,v/4.0).r*2.0-1.0;
}

float pyramid(float x)
{
   x=fract(x);
   return min(x*2.0,(1.0-x)*2.0);
}

float heightMap(vec2 p)
{
   p.x+=mod(floor(p.y),2.0)*0.5;
   return 1.0-min(pyramid(p.x),pyramid(p.y))+snoise(vec3(p*32.0,0.0))*0.2;
}

void main()
{
    v2f_o = vertex.xyz;
    
    float u=atan(vertex.y,vertex.x)/radians(180.0)*2.0;
    float v=vertex.z*0.25;
    float h=heightMap(vec2(u,v));
    
    v2f_h = h;
    
    v2f_o.xy=normalize(v2f_o.xy)*mix(4.0,5.0,h);
    
    v2f_v = (modelview * vec4(v2f_o,1.0)).xyz;
    v2f_n = mat3(modelview) * normal;
    
    gl_Position = projection * vec4(v2f_v, 1.0);
}
