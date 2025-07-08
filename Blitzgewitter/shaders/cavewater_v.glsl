#version 330

uniform mat4 projection, modelview;
uniform mat4 tetrahedra_planes[4];
uniform vec3 tetrahedra_origins[4];

in vec4 vertex;

out vec3 v2f_vo;
out vec3 v2f_o;

out float v2f_tetdist;

void main()
{
/*
    v2f_o = vertex.xyz;
    
    float mindist=1e4;
    for(int i=0;i<4;++i)
    {
        vec4 lo=vec4(vertex.xyz-tetrahedra_origins[i],1.0);
        if(lo.y>0.0)
            lo.y*=1000.0;
        else
            lo.y*=50.0;
        vec4 ds=max(vec4(0.0),tetrahedra_planes[i]*lo);
        float dist=sqrt(dot(ds*ds,vec4(1.0)));
        mindist=min(mindist,dist);
    }

    v2f_tetdist=mindist;
    
    vec4 v2=vertex;
    
    mindist=1e4;
    for(int i=0;i<4;++i)
    {
        vec4 lo=vec4(vertex.xyz-tetrahedra_origins[i],1.0);
        if(lo.y>0.0)
            lo.y*=512.0;
        else
            lo.y*=256.0;
        vec4 ds=max(vec4(0.0),tetrahedra_planes[i]*lo);
        float dist=sqrt(dot(ds*ds,vec4(1.0)));
        mindist=min(mindist,dist);
    }
    
    v2+=(1.0-smoothstep(0.0,0.02,mindist))*0.01;
   
    v2f_vo = (inverse(modelview) * vec4(0.0,0.0,0.0,1.0)).xyz;
    
    gl_Position = projection * modelview * v2;
    */

    gl_Position = projection * modelview * vertex;
}
