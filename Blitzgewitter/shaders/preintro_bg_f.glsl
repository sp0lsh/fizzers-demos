#version 330

noperspective in vec2 v2f_coord;

uniform sampler3D tex0;

uniform mat4 modelview;
uniform vec4 frustum;
uniform float znear;
uniform vec3 colour;

out vec4 output_colour;

float snoise(vec3 v)
{
    return texture(tex0,v/4.0).r*2.0-1.0;
}
void main()
{
   vec2 t=vec2(mix(frustum.x,frustum.y,v2f_coord.x),mix(frustum.z,frustum.w,v2f_coord.y));

   mat3 m=transpose(mat3(modelview));
   vec3 rd=m*normalize(vec3(t,-znear));
   
   output_colour.a=1.0;
   output_colour.rgb=colour;
}

