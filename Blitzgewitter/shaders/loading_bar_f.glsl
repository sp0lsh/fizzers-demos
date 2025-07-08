#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float aspect_ratio;
uniform float time;
uniform float fade;

noperspective in vec2 v2f_coord;

out vec4 output_colour0;

void main()
{
    vec2 coord=(v2f_coord-vec2(0.5))*vec2(aspect_ratio,1.0)*1.25+vec2(0.5);
    vec4 s=texture2D(tex0,coord);
    vec4 b=texture2D(tex2,coord);
    output_colour0.rgb=b.rgb*b.a+s.rgb*(smoothstep(0.0,0.004,1.01*time*67487.0/(2.0*65535.0)-texture2D(tex1,coord).r))*pow(s.a,1.5);
    output_colour0.rgb*=mix(1.0,0.0,clamp(fade,0.0,1.0));
}
