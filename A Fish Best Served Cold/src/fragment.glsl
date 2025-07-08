#version 130

float smin(float a,float b,float k){ return -log(exp(-k*a)+exp(-k*b))/k;}//from iq
float smax(float a,float b,float k){ return -smin(-a,-b,k);}

uvec2 rstate=uvec2(uvec2(gl_FragCoord.xy+gl_TexCoord[0].xy));

float dot2( in vec3 v ) { return dot(v,v); }
float dot2( in vec2 v ) { return dot(v,v); }

// from IQ
float sdCappedCone( in vec3 p, in float h, in float r1, in float r2 )
{
    vec2 q = vec2( length(p.xz), p.y );

    vec2 k1 = vec2(r2,h);
    vec2 k2 = vec2(r2-r1,2.0*h);
    vec2 ca = vec2(q.x-min(q.x,(q.y < 0.0)?r1:r2), abs(q.y)-h);
    vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot2(k2), 0.0, 1.0 );
    float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
    return s*sqrt( min(dot2(ca),dot2(cb)) );
}


// Tiny Encryption Algorithm for random numbers: 
uvec2 encrypt(uvec2 v)
{
    uint k[4],sum=0U,delta=0x9e3779b9U;
    k[0] = 0xA341316CU;
    k[1] = 0xC8013EA4U;
    k[2] = 0xAD90777DU;
    k[3] = 0x7E95761EU;
    for(uint i=0U;i<4U;++i)
    {
        sum += delta;
        v.x += ((v.y << 4U) + k[0]) ^ (v.y + sum) ^ ((v.y >> 5U) + k[1]);
        v.y += ((v.x << 4U) + k[2]) ^ (v.x + sum) ^ ((v.x >> 5U) + k[3]);
    }

    return v;
}

float rand()
{
    rstate=encrypt(rstate);
    return float(rstate.x&0xfffffU)/(1U<<20U);
}


/*   NIKAT'S 3D SIMPLEX NOISE   */
vec3 hash3(vec3 c){
    float j = 4096.0*sin(dot(c,vec3(17, 59.4, 15)));
    return vec3(fract(64.0*j), fract(8.0*j), fract(512.0*j))-0.5;
}

float simplex(vec3 p){
    vec3 s = floor(p + dot(p, vec3(1.0/3.0)));
    vec3 x = p - s + dot(s, vec3(1.0/6.0));
    vec3 e = step(vec3(0), x - x.yzx);
    vec3 i1 = e*(1.0 - e.zxy);
    vec3 i2 = 1.0 - e.zxy*(1.0 - e);
    vec3 x1 = x - i1 + vec3(1.0/6.0);
    vec3 x2 = x - i2 + vec3(1.0/3.0);
    vec4 w = pow(max(0.6 - vec4(dot(x, x), dot(x1, x1), dot(x2, x2), dot(x-0.5,x-0.5)), 0.0),vec4(4));
    return dot(w*vec4(dot(hash3(s),x),dot(hash3(s+i1),x1),dot(hash3(s+i2),x2),dot(hash3(s+1.0),x-0.5)),vec4(52));
}

mat2 rotmat(float a)
{
    return mat2(cos(a),sin(a),-sin(a),cos(a));
}

float sdEllipsoid( in vec3 p, in vec3 r )
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0)/k1;
}

float eyes(vec3 p)
{
    p.x=abs(p.x);
    return mix(.1,1.,step(0,distance(p,vec3(.2,.65,.3))-.05));
}

vec2 U(vec2 a,vec2 b)
{
    return b.x<a.x?b:a;
}

float feet;
vec2 penguinf(vec3 p)
{
    vec3 op=p;
    vec2 d=vec2(20,1);

    p.y-=.1;

    d.x=min(d.x,sdEllipsoid(p-vec3(0,.5,0),vec3(.5,.6,.5)/2));

    d.x=smin(d.x,sdEllipsoid(p-vec3(0,.1,0.),vec3(.4)),7);

    d.x=smax(d.x,-p.y-.1,20);

    d.x=min(d.x,sdEllipsoid(p-vec3(0,.5,0.2),vec3(.2,.13,.32))+.06);

    p.x=abs(p.x);

    vec3 p2=p;
    p2.xz*=rotmat(-.2);   
    float feetd=smax(-p2.y-.1,sdEllipsoid(p2-vec3(.17,-.07,0.1),vec3(.2,.14,.35)),90);

    p.xy*=rotmat(.35);
    d.x=smin(d.x,sdEllipsoid(p-vec3(.46,.15,0.),vec3(.07,.2,.1)),60);

    feet=0.;
    if(feetd<d.x)feet=1.;
    d.x=min(d.x,feetd);

    return d;
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
    vec3 d = abs(p) - b;
    return length(max(d,0.0)) - r
        + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 
}


vec2 f(vec3 p,int noice)
{
    float an=1.;

    vec2 d=vec2(20,0);

    float roomd=20;

    roomd=smax(-(length(p.xz)-7.),-(length(vec2((rotmat(-.55)*p.xz).x,p.y+.5))-1),32);

    roomd=smin(roomd,p.y+.9,4.);

    p.y+=.9;
    vec2 penguind=penguinf(p);

    // hat
    penguind=U(penguind,vec2(length(vec2(max(0.,abs(p.y-.8)-.2),max(0.,length(p.xz)-.15)))-.05,2));
    penguind=U(penguind,vec2(length(vec2(max(0.,abs(p.y-.85)-.05),max(0.,length(p.xz)-.27)))-.05,2));

    roomd=smin(roomd,length(vec2(max(0.,abs(p.y-.1)-.2),max(0.,length(p.xz-.93)-.5)))-.05,32);

    d=U(d,vec2(roomd,0));
    d=U(d,penguind);

    // fish
    float fishd;
    vec3 p2=p-vec3(1.06,.7,.8);
    p2.xz*=rotmat(.7);
    fishd=smin( sdEllipsoid(p2,vec3(.5,.25,.2)/2.5),
               sdCappedCone(-(p2-vec3(-0.2,0,0)).zxy,.1,0.,.1), 140);
    fishd+=simplex(p*22)*.01;

    d=U(d,vec2(fishd,4));

    d.x+=simplex(p*150)*.0005;
    d.x+=simplex(p*18)*.002;
    d.x+=simplex(p*48)*.00075;
    d.x+=simplex(p*6)*.004;
    d.x+=simplex(p*2)*.02;

    if(noice==0)
    {
        p-=vec3(1,.7,.8);
        p.xz*=rotmat(.45);
        float iced=abs(sdRoundBox(p,vec3(.3),.0)-.05)-.015;
        iced+=simplex(p*2)*.02;
        iced+=simplex(p*6)*.004;
        iced+=simplex(p*58)*.025*(pow(simplex(p*3+3)+simplex(p*6)/2.,3.)+0.05);
        d=U(d,vec2(iced,3));
    }

    d.x=min(d.x,20);

    return d;
}

float dist(vec3 p){return f(p,0).x;}

vec3 distG(vec3 p)
{
    vec2 e=vec2(1e-5,0);
    return vec3(dist(p+e.xyy),dist(p+e.yxy),dist(p+e.yyx))-
        vec3(dist(p-e.xyy),dist(p-e.yxy),dist(p-e.yyx));
}


float calcAO( const vec3 pos, const vec3 nor ) {
    float aodet=0.001;
    float totao = 0.0;
    float sca = 2.5;
    for( int aoi=0; aoi<60; aoi++ ) {
        float hr = aodet*float(aoi*aoi);
        vec3 aopos =  nor * hr + pos;
        float dd = f( aopos, 0 ).x;
        totao += -(dd-hr)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - 5.0*totao, 0., 1.0 );
}


// Soft shadow for SDF, from IQ and Sebastian Aaltonen:
// https://www.shadertoy.com/view/lsKcDD
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax)
{
    float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    for( int i=0; i<32; i++ )
    {
        float h = f( ro + rd*t, 1 ).x;

		// use this if you are getting artifact on the first iteration, or unroll the
		// first iteration out of the loop
		//float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

		float y = h*h/(2.0*ph);
		float d = sqrt(h*h-y*y);
		res = min( res, 0.75*d/max(0.0,t-y) );
		ph = h;

        t += h;

        if( res<0.0001 || t>tmax )
		{
			break;
		}

    }
    return clamp( res, 0.0, 1.0 );
}

vec2 disc(vec2 uv)
{
    float a = uv.x * acos(-1.) * 2;
    float r = sqrt(uv.y);
    return vec2(cos(a), sin(a)) * r;
}



void sampleImage(vec2 p)
{
    float an=3.6;

    vec3 cam=vec3(-0.7,-.3,6.);
    vec2 lens=disc(vec2(rand(),rand()));
    vec3 ro=cam+vec3(lens,0)*.03;
    vec3 rt=cam+vec3(p.xy*1.5,-6.);
    vec3 rd=normalize(rt-ro);

    rd.xz=mat2(cos(an),sin(an),sin(an),-cos(an))*rd.xz;
    ro.xz=mat2(cos(an),sin(an),sin(an),-cos(an))*ro.xz;

    float s=20.;
    vec3 icespec=vec3(0);
    vec3 absorp=vec3(1);
    float t=0.,d=0.,m=+1,id=0;
    for(int i=0;i<350;++i)
    {
        vec2 r=f(ro+rd*t, 0);
        d=r.x;
        id=r.y;
        if(abs(d)<1e-5)
        {
            if(r.y==3.)
            {
                t+=1e-4;
                vec3 n=normalize(distG(ro+rd*t));
                rd=rand()>.8 ? reflect(rd,n) : (m>0.?refract(rd,n*-sign(dot(rd,n)),1./1.01):rd);
                 m=-m;
                icespec+=pow(max(0.,dot(n,normalize(-rd + normalize(vec3(4.5,2,1))))),120.)*18;
                icespec+=pow(max(0.,dot(n,normalize(-rd + normalize(vec3(4.5,2,3))))),220.)*12;
                absorp*=vec3(.95,.95,1);
            }
            else
			{
				break;
			}
        }
        t+=d*.8*m;
        if(t>25.)
		{
			break;
		}
    }

    if(t>25.)
    {
        gl_FragColor.rgb=vec3(.25)*0;
    }   
    else
    {
        vec3 rp=ro+rd*t;
        vec3 n=normalize(distG(rp));

        gl_FragColor.rgb=vec3(1-dot(-rd,n))*.3+eyes(rp-vec3(0,-.9,0))*(.75+.3*dot(n,normalize(vec3(1))))*1.1;
        rp.y+=.9;
        if(id>.5)
        {
            gl_FragColor.rgb*=mix(vec3(1,.7,.4), mix(vec3(1),vec3(.5,.5,1),max(step(0.,-rp.z),step(.2,length((rp.xy-vec2(0,.26)) * vec2(1,.99) )))),
                                  max(step(0.,-rp.z),step(.2,length((rp.xy-vec2(0,.6)) * vec2(1.5,3) ))));

            if(feet>0.5)
			{
                gl_FragColor.rgb=vec3(1,.7,.4);
			}
        }
        else
        {
            gl_FragColor.rgb*=vec3(.7,.75,1);
        }

        if(id==2)
		{
            gl_FragColor.rgb=vec3(.7,1,.4)/2;
		}
		
        if(id==4)
        {
            gl_FragColor.rgb=vec3(1.,.4,.2)*2*smoothstep(0.5,.8,rp.y);
            if((length(rp.xy-vec2(1.12,.73)))<.03)
			{
                gl_FragColor.rgb=vec3(1);
			}
			
            if((length(rp.xy-vec2(1.12,.73)))<.02)
			{
                gl_FragColor.rgb=vec3(.01);
			}
        }

        rp.y-=.9;

        gl_FragColor.rgb*=mix(.4,1.,calcSoftshadow(rp,normalize(vec3(1,2,1)),1e-4,4.))*1.1;

        gl_FragColor.rgb*=mix(.1,1.,pow(clamp(calcAO(rp,normalize(n+0.25)),0,1),2.5));

        gl_FragColor.rgb*=1.+simplex(rp*6)*.1;

        gl_FragColor.rgb*=1-smoothstep(0.,8.,length(rp))+smoothstep(9.,19.,length(rp))*smoothstep(0.,1.,-rp.y+rp.z/2+5)/2;
        gl_FragColor.rgb+=icespec;
    }
    gl_FragColor.rgb=max(gl_FragColor.rgb*absorp,0.);
    gl_FragColor.rgb+=.01;

    gl_FragColor.rgb-=vec3(dot(gl_FragColor.rgb,vec3(1./3.)))*.1;

}


void main()
{
	vec2 t=((gl_FragCoord.xy+vec2(rand(),rand())*1.5)/vec2(1920,1080)*2-1.);
	if(abs(t.x)>2./3.||abs(t.y)>1.)
		discard;
   t*=1.5;
	t.y/=1920./1080.;
	vec2 p=t.xy;

	vec3 c=vec3(0);
	gl_FragColor.rgb=vec3(0);
	sampleImage(p);
	c+=clamp(gl_FragColor.rgb,0.,1.);

	gl_FragColor.rgb=c;
	gl_FragColor.rgb*=1.-(pow(abs(p.x),3.)*2+pow(abs(p.y*1.5),3.))*.15;

	gl_FragColor.rgb+=vec3(1,.5,.2)*.01;

	gl_FragColor.rgb+=vec3(1,.9,.9)*simplex(vec3(p*200,0))*.002;

	gl_FragColor.rgb/=(gl_FragColor.rgb+.75)*.55;
	gl_FragColor.rgb=pow(gl_FragColor.rgb,vec3(1./2.2));
	
	gl_FragColor /= gl_TexCoord[0].z;

}

