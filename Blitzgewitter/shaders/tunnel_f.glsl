#version 330

uniform sampler3D tex0;

uniform float background;
uniform float fade;
uniform float z_offset;

in vec3 v2f_o;
in float v2f_h;
in vec3 v2f_v;

out vec4 output_colour;

float snoise(vec3 v)
{
    return texture(tex0,v/4.0).r*2.0-1.0;
}

vec3 tunnelColour(vec3 p,float h)
{
   p.z+=90.0;

   if(p.z < -700)
    return vec3(0.0);

   p+=vec3(snoise(floor(p*16.0*8.0)/16.0))*0.002;
    
   float ang=atan(p.x,p.y);

   float s=fract(ang/radians(90.0)*4.0+p.z*0.1);
   float sx=step(mix(0.5, 1.0, snoise(p * 2.0)), smoothstep(-60.0,-140.0,p.z) - smoothstep(-300.0,-310.0,p.z));
   float ss=mix(0.0,0.01,sx);
   vec3 stripes0=(smoothstep(0.0,ss,s)-smoothstep(ss*2.0,ss*3.0,s))*vec3(1.0,0.9,0.5)*2.0*sx*1.5*pow(h,8.0);

   s=fract(ang/radians(90.0)*1.0+p.z*0.2);
   sx=step(mix(0.4, 1.0, snoise(p)), smoothstep(-20.0,-80.0,p.z)-smoothstep(-300.0,-310.0,p.z));
   ss=mix(0.0,0.01,sx);
   vec3 stripes1=(smoothstep(0.0,ss,s)-smoothstep(ss*2.0,ss*3.0,s))*vec3(1.0,0.9,0.6)*3.0*sx*1.5*pow(h,8.0);
   
   vec3 nfp=-abs(fract(p*8.0)-vec3(0.5));
   float nf=mix(0.75,1.0,smoothstep(-0.46,-0.45,nfp.x)*smoothstep(-0.46,-0.45,nfp.y)*smoothstep(-0.46,-0.45,nfp.z))*0.75+pow(1.0-smoothstep(0.0,0.5,length(nfp)),2.0)*0.5;
   
   sx = smoothstep(-240.0,-310.0,p.z) - smoothstep(-460.0,-540.0,p.z);
   float f = step(snoise(floor(p*2.0)/2.0)*0.5+0.5,sx);
   vec3 noi0 = ((vec3(1.0,1.0,0.8)*smoothstep(0.4,1.0, (1.0-floor(h*4.0)/4.0+snoise(floor(p))+snoise(floor(p*4.0))*0.2) * f )*5.0+
            mix(0.1,1.0,(1.0-h))*mix(vec3(0.1,0.1,0.2).bgr*1.1,vec3(1.0,1.2,1.3).bgr,0.5))*0.06*f) * nf;
   
   vec3 noi1 = max(vec3(0.0),(vec3(smoothstep(0.5,0.8,snoise(p*2.0)+snoise(p*5.0)*0.5)) + vec3(smoothstep(0.5,0.8,snoise(p*4.0)+snoise(p*7.0)*0.4))*2.4*pow(snoise(p)*0.5+0.5,4.0))-vec3(0.6)) * smoothstep(-470.0,-550.0,p.z);
   
   return max(vec3(0.0),stripes1 + stripes0 + noi0 + noi1);
}


void main()
{
   vec3 rp=v2f_o.xyz;
   float l=1.0-smoothstep(10.0,70.0,length(v2f_v));
   output_colour.a=1.0-smoothstep(0.0,30.0,length(v2f_v));
   output_colour.rgb=max(background,output_colour.a)*l*tunnelColour(rp+vec3(0.0,0.0,z_offset),v2f_h)*fade;
}

