#version 330

uniform sampler2D tex0, depth_tex;
uniform sampler2D tex1;
uniform vec4 tint;
uniform float darken;
uniform float time;
uniform int frame_num;
uniform float aspect_ratio;

noperspective in vec2 v2f_coord;

out vec4 output_colour;
 
vec2 unitCubeInterval(vec3 ro, vec3 rd)
{
    vec3 slabs0 = (vec3(+1.0) - ro) / rd;
    vec3 slabs1 = (vec3(-1.0) - ro) / rd;

    vec3 mins = min(slabs0, slabs1);
    vec3 maxs = max(slabs0, slabs1);

    return vec2(max(max(mins.x, mins.y), mins.z),
                min(min(maxs.x, maxs.y), maxs.z));
}

vec3 rotateX(float a, vec3 v)
{
    return vec3(v.x, cos(a) * v.y + sin(a) * v.z, cos(a) * v.z - sin(a) * v.y);
}

vec3 rotateY(float a, vec3 v)
{
    return vec3(cos(a) * v.x + sin(a) * v.z, v.y, cos(a) * v.z - sin(a) * v.x);
}

float wipe(vec2 t)
{
    float tt = time * 1.5;
    float m = 0.0;
   
    if(tt > 1.0)
        return 1.0;

    for(int i = 0; i < 5; ++i)
    {
      float angx = tt * 3.0 + float(i), angy = tt + float(i);
      vec3 ro = rotateY(angy, rotateX(angx, vec3(cos(float(i) * 10.0) * 1.5, 30.0 + float(i) * 10.0 - tt * 40.0,
               sin(float(i) * 10.0) * 1.5))) + vec3(0.0, 0.0, -1.0),
               rd = rotateY(angy, rotateX(angx, rotateX(-1.3, vec3(t.xy, -1.0))));

      vec2 is = unitCubeInterval(ro, rd);

      float f = step(is.x, is.y) * step(is.x, 0.0);

      m = max(m, f);
    }
    
    return m;
}
 
void main()
{
    output_colour = texture2D(tex0, v2f_coord) * mix(1.0, 1.0 - length(v2f_coord - vec2(0.5)), darken);
    output_colour *= tint;
    output_colour.rgb += output_colour.rgb * texelFetch(tex1, ivec2(mod(gl_FragCoord.xy + ivec2(frame_num * ivec2(23, 7)), textureSize(tex1, 0))), 0).rrr * 0.3;
    output_colour = pow(max(vec4(0.0), output_colour), vec4(0.6));    
    output_colour.rgb = mix(vec3(1.0), (output_colour.rgb - vec3(0.5)) * 1.2 + vec3(0.5), wipe((v2f_coord - vec2(0.5)) * vec2(2.0 * aspect_ratio, 2.0)));
}

