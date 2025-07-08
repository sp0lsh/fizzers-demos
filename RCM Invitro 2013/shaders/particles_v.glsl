#version 330

uniform mat4 projection, modelview;
uniform float disappear;
uniform float time;
uniform float arp;

in vec4 vertex;
in vec2 coord;

out VertexOut
{
    noperspective vec2 tc;
    
} v_out;

void main()
{
    v_out.tc = coord;
    vec4 vpos = modelview * vertex;
    
    //vpos.xyz += (-sin(vpos.zyx*0.07)*30.0 + sin(vpos.zxy*0.1)*40.0)*8.0*pow(disappear, 4.0)*vec3(4.0,1.0,1.0);
    vpos.xy += (cos(vpos.yx * 0.03) * 1000.0 + vpos.xy * pow(length(vpos.xy), 2.0) * 0.001) * pow(disappear, 4.0);
    
    vpos.z += cos(vpos.y * 0.1 + time * 20.0) * sin(vpos.x * 0.04 + time * 10.0) * arp;
    
    gl_Position = projection * vpos;
}

