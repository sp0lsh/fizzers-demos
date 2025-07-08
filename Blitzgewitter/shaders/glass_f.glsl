#version 330

in vec2 v2f_coord;

in vec4 w_pos;

in vec3 v2f_v, v2f_n;
in vec3 v2f_o;

noperspective in vec2 v2f_screen;
noperspective in vec2 v2f_zw;

uniform sampler2D tex1;

uniform float background;
uniform float rayjitter;
uniform sampler3D tex0;

uniform mat4 modelview;
uniform vec4 frustum;
uniform float znear;
uniform float burn_up;

uniform mat4 tetrahedra_mats[4];
uniform mat4 tetrahedra_mats_inv[4];
uniform mat4 modelview_inv;

uniform float time;

out vec4 output_colour;

const mat4 tetrahedronPlanes=mat4(0.000000, 0.000000, 0.816496, -0.816496,
-1.000000, 0.333334, 0.333333, 0.333333,
0.000000, 0.942809, -0.471405, -0.471405,
-0.666000, -0.444000, -0.111000, -0.111000);


float snoise(vec3 v)
{
    return texture(tex0,v/4.0).r*2.0-1.0;
}

float traceCircle(vec2 ro,vec2 rd,vec2 co,float r)
{
   float a=dot(rd,rd);
   float b=dot(rd,ro-co);
   float c=dot(rd,rd)*(dot(ro-co,ro-co)-r*r);
   float d=b*b-c;

   if(d<0.0)
      return 1e4;

   return (-b+sqrt(d))/dot(rd,rd);
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

vec2 texcoordForNormal(vec3 p,vec3 n)
{
    p-=vec3(0.000000, 0.666667, 0.235702);
    vec3 u=normalize(cross(n,vec3(0.0,-1.0,0.0)));
    vec3 v=normalize(cross(n,u));
    u=normalize(cross(v,n));
    return vec2(dot(p,u),dot(p,v))/vec2(1.6,1.42)+vec2(0.5,1.0);
}

vec3 tetrahedronNormal(vec3 cp, out vec2 tc)
{
    vec4 d0=tetrahedronPlanes*vec4(cp,1.0);

    vec3 n=vec3(0.0);
    for(int i=0;i<4;++i)
    {
        if(d0[i]>-0.1e-3)
        {
            n=transpose(tetrahedronPlanes)[i].xyz;
        }
    }

    tc=texcoordForNormal(cp,normalize(n));
    return normalize(n+snoise(cp*16.0)*0.002+snoise(cp*1.0)*0.0125);
}


float tetrahedronTrace(vec3 ro, vec3 rd)
{
    vec4 d0=tetrahedronPlanes*vec4(ro,1.0);
    vec4 d1=tetrahedronPlanes*vec4(rd,0.0);

    vec4 t=-d0/d1;
    
    float t1=1e4,t0=-1e4;
    for(int i=0;i<4;++i)
    {
        if(d1[i]<0)
            t0=max(t0,t[i]);
        else
            t1=min(t1,t[i]);
    }
   

   if(t1<t0 || (t0<0.0 && t1<0.0))
      return 1e4;
   else if(t0<0.0)
      return t1;
   else
      return t0;
    

}

float tetrahedronTrace(vec3 ro, vec3 rd, int tet_idx, out vec3 n, out vec2 tc)
{
    vec3 ro2=((tetrahedra_mats_inv[tet_idx]) * vec4(ro,1.0)).xyz;
    vec3 rd2=mat3((tetrahedra_mats_inv[tet_idx]))*rd;
    float lt=tetrahedronTrace(ro2, rd2);
    n=mat3(tetrahedra_mats[tet_idx])*tetrahedronNormal(ro2+rd2*lt, tc);
    return lt;
}

float sceneTrace(vec3 ro, vec3 rd,out vec3 n,out int tet_idx,out vec2 tc)
{
    float t=1e4;
    for(int i=0;i<4;++i)
    {
        vec3 ro2=((tetrahedra_mats_inv[i]) * vec4(ro,1.0)).xyz;
        vec3 rd2=mat3((tetrahedra_mats_inv[i]))*rd;
        float lt=tetrahedronTrace(ro2, rd2);
        if(lt<t)
        {
            n=mat3(tetrahedra_mats[i])*tetrahedronNormal(ro2+rd2*lt, tc);
            t=lt;
            tet_idx=i;
        }
    }
    
    return t;
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


vec4 traceTunnel(vec3 ro,vec3 rd)
{
   float r0=4.0;
   float r1=5.0;

   float d0=traceCircle(ro.xy,rd.xy,vec2(0.0),r0);
   float d1=traceCircle(ro.xy,rd.xy,vec2(0.0),r1);

   float u=0.0,v=0.0,h=0.0;
   float ph=0.0;
   vec3 rp=vec3(0.0);

   {
      float d=d0;
      rp=ro+rd*d;
      u=atan(rp.y,rp.x)/radians(180.0)*2.0;
      v=rp.z*0.25;
      ph=h;
      h=heightMap(vec2(u,v));
   }
   
   vec4 col=vec4(0.0);

    col.rgb=tunnelColour(rp,h);
    
   if(background<0.5)
   {
    col.a=1.0-smoothstep(0.0,40.0,length(ro-rp));
    col.rgb*=col.a;
   }
   
   return col;
}

vec4 scene(float ior)
{
   vec2 coord=v2f_screen*0.5+vec2(0.5);
   vec2 t=vec2(mix(frustum.x,frustum.y,coord.x),mix(frustum.z,frustum.w,coord.y));

   mat3 m=transpose(mat3(modelview));
   vec3 ro=((modelview_inv)*vec4(0.0,0.0,0.0,1.0)).xyz,rd=m*normalize(vec3(t,-znear));

   vec3 cpn=vec3(1.0,0.0,0.0),cpn2=vec3(1.0,0.0,0.0);
   vec2 tc=vec2(0.0),tc2=vec2(0.0);
   int tet_idx=0;
   float ct=sceneTrace(ro,rd,cpn,tet_idx,tc);

   vec4 col=vec4(0.0);
   
   if(ct<1e3)
   {
      vec3 cp=ro+rd*(ct+1e-4);
       
      vec3 rrd=reflect(rd,cpn);
   
      vec3 rd2=refract(rd,cpn,1.0/ior);
      float ct2=tetrahedronTrace(cp,rd2,tet_idx,cpn2,tc2);
      cpn2=-cpn2;
      vec3 cp2=cp+rd2*(ct2+1e-3);
   
      vec3 rd3=refract(rd2,cpn2,ior);

      if(rd3.x==0.0)
      {
         rd3=reflect(rd2,cpn2);
      }

      vec4 c0=traceTunnel(cp2,rd3);
      vec4 c1=traceTunnel(cp,rrd);

      vec3 texc0=texture2D(tex1,tc).rgb*1.5;
      vec3 texc1=texture2D(tex1,tc2).rgb*1.5;
      col=mix(c0*mix(vec4(1.0)*vec4(vec3(1.0),1.0),vec4(1.0),exp(-ct2*4.0)),c1*4.0,pow(max(0.0,1.0-dot(cpn,-rd)),4.0));
      col.rgb+=texc0*0.2+texc1*0.1;
      col.a=1.0;
   }

   return col;
}

void main()
{
   output_colour=clamp(scene(1.5),vec4(0.0),vec4(100.0));
}

