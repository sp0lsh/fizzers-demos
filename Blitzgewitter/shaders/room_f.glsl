#version 330

uniform sampler3D volume_tex;
uniform sampler3D noise_tex;
uniform mat4 inv_object;

in vec3 g2f_n;
in vec3 g2f_v;
in vec3 g2f_o;
in vec3 g2f_bcoord;

out vec4 output_colour0;

uniform vec3 box_min;
uniform vec3 box_max;

uniform vec3 material_colours[3];

vec3 perturbedNormal()
{
    vec3 n=normalize(g2f_n);
    vec3 d=texture(noise_tex,g2f_o*2.0).rgb;
    return normalize(n+(d-dot(n,d)*n)*0.04);
}

float fbm(vec3 p)
{
    float f=0.0;
    for(int i=0;i<3;++i)
        f+=texture(noise_tex,g2f_o*exp2(i)).r/exp2(i+1);
    return f;
}

vec2 intersectBox(vec3 ro, vec3 rd)
{
    vec3 slabs0 = (box_max*3.0 - ro) / rd;
    vec3 slabs1 = (box_min*3.0 - ro) / rd;

    vec3 mins = min(slabs0, slabs1);
    vec3 maxs = max(slabs0, slabs1);

    return vec2(max(max(mins.x, mins.y), mins.z),
                min(min(maxs.x, maxs.y), maxs.z));
}

void main()
{
    vec3 ro=(inv_object*vec4(g2f_v,1.0)).xyz*3.0;
    vec3 pn=perturbedNormal();
    vec3 rd=(mat3(inv_object)*reflect(normalize(g2f_v), pn))*3.0;

    vec2 is=intersectBox(ro, rd);

    if(is.x>is.y)
    {
        output_colour0.rgb=vec3(0.0);
        return;
    }
    
    is.x=max(0.0,is.x);

    float alpha=1.0;
    vec3 col=vec3(0.0);
    
    float gloss=1.0+smoothstep(0.4,0.5,fbm(g2f_o));

    ro=(ro-box_min)/(box_max-box_min);
    rd=(rd-box_min)/(box_max-box_min);
    
    for(int i=0;i<64;i+=1)
    {
        float t=mix(is.x,is.y,float(i)/64.0);
        vec3 rp=ro+rd*t;
        vec4 samp=textureLod(volume_tex,rp,gloss*log2(8.0*t));
        col+=samp.rgb*samp.a*alpha;
        alpha*=1.0-samp.a;
    }
    
    col=material_colours[0]*col.r+material_colours[1]*col.g+material_colours[2]*col.b;
    
    float fres=pow(max(0.0,1.0-dot(normalize(g2f_v),pn)),2.0);
    
    float mb=min(g2f_bcoord.x,min(g2f_bcoord.y,g2f_bcoord.z));
    
    output_colour0.rgb=mix(col*0.2*(1.0+fres),vec3(0.0),(1.0-smoothstep(0.002,0.002+fbm(g2f_o)*0.01,mb)));
    output_colour0.a=1.0;
}
