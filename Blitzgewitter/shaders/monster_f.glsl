#version 330

uniform sampler2D tex0;
uniform vec2 screen_res;
uniform float time;
uniform float switch_view_time;

noperspective in vec2 v2f_coord;

out vec4 output_colour0;

void main()
{
    if(time < switch_view_time)
    {
        vec2 coord=v2f_coord+vec2(-0.33,-0.7+(time/(time+0.6))*0.5);
        vec2 sz=vec2(640.0,480.0);
        output_colour0 = texture2D(tex0,coord*screen_res/sz*1.25)*vec4(vec3(0.33),1.0);
    }
    else
    {
        float z2=mix(0.8,0.6,(time-switch_view_time)*0.05);
        vec2 sz=vec2(640.0,480.0);
        vec2 coord=(v2f_coord-vec2(0.5))*screen_res/sz*mix(z2,0.01,clamp(pow(max(0.0,(time-20.0)*0.3),2.0),0.0,1.0));
        coord+=vec2(825.0,1200-516.0)/vec2(1600.0,1200.0);
        output_colour0 = texture2D(tex0,coord)*vec4(vec3(0.33),1.0);
    }
}
